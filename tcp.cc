#include <unistd.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string>
#include <print>
#include <iostream>

constexpr int MAX_KEVENTS = 32;
constexpr int MAX_BACKLOG = 32;

// Helper function to set a file descriptor to non-blocking mode
static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Static kevent struct
static struct kevent k;

// Helper function to initialize a kqueue on the input file descriptor
static int init_kqueue(int fd) {
    int q = kqueue();
    if (q == -1)
        throw errno;

    EV_SET(&k, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(q, &k, 1, nullptr, 0, nullptr) == -1)
        throw errno;

    return q;
}

namespace tcp {
    struct conn {
        int fd, kq;
        sockaddr_in addr;
        socklen_t len;
        struct kevent list[MAX_KEVENTS];

        explicit conn(int fd, int kq) : fd(fd), kq(kq) {}

        static conn New(int fd) {
            return conn{ fd, init_kqueue(fd) };
        }

        std::string Read() {
            int nev = kevent(kq, nullptr, 0, list, MAX_KEVENTS, nullptr);
            if (nev == -1)
                throw errno;

            for (int i = 0; i < nev; i++) {
                if (list[i].filter == EVFILT_READ) {
                    char buffer[128];
                    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
                    if (bytesRead <= 0)
                        throw errno;

                    buffer[bytesRead] = '\0'; // Terminate string
                    return std::string(buffer);
                }
            }
            throw - 1;
        };

        void Write(const std::string& message) {
            if (send(fd, message.c_str(), message.size(), MSG_DONTWAIT) == -1)
                throw errno;
        }
    };

    struct listener {
        int fd, kq;
        struct kevent list[MAX_KEVENTS];

        explicit listener(int fd, int kq) : fd(fd), kq(kq) {}

        static listener New(int fd) {
            return listener{ fd, init_kqueue(fd) };
        }

        // Accept incomming connections (blocks)
        conn Accept() {
            int nev = kevent(kq, nullptr, 0, list, MAX_KEVENTS, nullptr);
            if (nev == -1)
                throw errno;

            for (int i = 0; i < nev; i++) {
                if (list[i].filter == EVFILT_READ) {
                    sockaddr_in addr;
                    socklen_t len;
                    int connfd = accept(fd, (sockaddr*)&addr, &len);
                    if (connfd == -1)
                        continue; // <- might consider throwing here
                    set_nonblocking(connfd);

                    return conn::New(connfd);
                }
            }
            throw - 1;
        }
    };

    // Create a socket and bind to the port, throws an error if any of the syscalls fail
    listener Listen(int port) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
            throw errno;
        set_nonblocking(fd);

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) == -1)
            throw errno;

        if (listen(fd, MAX_BACKLOG) == -1)
            throw errno;

        return listener::New(fd);
    }
}