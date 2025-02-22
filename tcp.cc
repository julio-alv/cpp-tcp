#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "event.cc"

constexpr int MAX_BACKLOG = 32;
constexpr int BUFF_SIZE = 1024;

// Helper function to set a file descriptor to non-blocking mode
static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

namespace tcp {
    struct conn {
        int fd;
        sockaddr_in addr;
        socklen_t len;

        event::queue q {0};

        explicit conn(int fd) : fd(fd) {
            q = event::queue {fd};
        }

        auto Read() -> std::vector<std::byte> {
            q.rx();
            std::vector<std::byte> buff(BUFF_SIZE);
            ssize_t n = recv(fd, buff.data(), buff.size(), MSG_DONTWAIT);
            if (n <= 0)
                throw strerror(errno);

            buff.erase(buff.begin() + n, buff.end());
            return buff;
        };

        void Write(const std::vector<std::byte>& msg) {
            if (send(fd, msg.data(), msg.size(), MSG_DONTWAIT) == -1)
                throw strerror(errno);
        }
    };

    struct listener {
        int fd;
        event::queue q {0};

        explicit listener(int fd) : fd(fd) {
            q = event::queue {fd};
        }

        // Accept incomming connections
        conn Accept() {
            q.rx();
            sockaddr_in addr;
            socklen_t len;
            int cfd = accept(fd, (sockaddr*)&addr, &len);
            if (cfd == -1)
                throw strerror(errno);
            set_nonblocking(cfd);

            return conn{ cfd };
        }
    };

    // Create a socket and bind to the port
    listener Listen(int port) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
            throw strerror(errno);
        set_nonblocking(fd);

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) == -1)
            throw strerror(errno);

        if (listen(fd, MAX_BACKLOG) == -1)
            throw strerror(errno);

        return listener{ fd };
    }
}