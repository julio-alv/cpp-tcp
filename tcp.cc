#include <unistd.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <expected>
#include <string>
#include <print>

// Helper function to set a file descriptor to non-blocking mode
static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Static kevent struct
static struct kevent k;

// Helper function to initialize a kqueue on the input file descriptor
static auto init_kqueue(int fd) -> std::expected<int, int> {
    int q = kqueue();
    if (q == -1)
        return std::unexpected(errno);

    EV_SET(&k, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(q, &k, 1, nullptr, 0, nullptr) == -1)
        return std::unexpected(errno);

    return q;
}

namespace tcp {
    struct Conn {
        int fd, kq;
        sockaddr_in addr;
        socklen_t len;
        struct kevent list[32];

        explicit Conn(int fd, int kq) : fd(fd), kq(kq) {}

        static auto New(int fd) -> std::expected<Conn, int> {
            auto q = init_kqueue(fd);
            if (!q)
                return std::unexpected(q.error());

            return Conn{ fd, q.value() };
        }

        auto Read() -> std::expected<std::string, int> {
            int nev = kevent(kq, nullptr, 0, list, 32, nullptr);
            if (nev == -1)
                return std::unexpected(errno);

            for (int i = 0; i < nev; i++) {
                if (list[i].filter == EVFILT_READ) {
                    char buffer[128];
                    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
                    if (bytesRead <= 0)
                        return std::unexpected(errno);

                    buffer[bytesRead] = '\0'; // Terminate string
                    return std::string(buffer);
                }
            }
            return std::unexpected(-1);
        };

        auto Write(const std::string& message) -> std::expected<bool, int> {
            if (send(fd, message.c_str(), message.size(), MSG_DONTWAIT) == -1)
                return std::unexpected(errno);

            return true;
        }
    };

    struct Listener {
        int fd, kq;
        struct kevent list[32];

        explicit Listener(int fd, int kq) : fd(fd), kq(kq) {}

        static auto New(int fd) -> std::expected<Listener, int> {
            auto q = init_kqueue(fd);
            if (!q)
                return std::unexpected(q.error());

            return Listener{ fd, q.value() };
        }

        // Accept incomming connections (blocks)
        auto Accept() -> std::expected<Conn, int> {
            int nev = kevent(kq, nullptr, 0, list, 32, nullptr);
            if (nev == -1)
                return std::unexpected(errno);

            for (int i = 0; i < nev; i++) {
                if (list[i].filter == EVFILT_READ) {
                    sockaddr_in addr;
                    socklen_t len;
                    int connfd = accept(fd, (sockaddr*)&addr, &len);
                    if (connfd == -1)
                        continue;
                    set_nonblocking(connfd);

                    auto conn{ Conn::New(connfd) };
                    if (!conn)
                        continue;

                    return conn;
                }
            }
            return std::unexpected(-1);
        }
    };

    // Create a socket and bind to the port, returns an error if any of the syscalls fail
    auto Listen(int port) -> std::expected<Listener, int> {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
            return std::unexpected(errno);
        set_nonblocking(fd);

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0)
            return std::unexpected(errno);

        if (listen(fd, 32) == -1)
            return std::unexpected(errno);

        return Listener::New(fd);
    }
}