#include <sys/event.h>
#include <iostream>

constexpr int MAX_KEVENTS = 32;

static struct kevent k;

namespace event {
    struct queue {
        int q;
        struct kevent list[MAX_KEVENTS];

        explicit queue(int fd) {
            q = kqueue();
            if (q == -1)
                throw errno;

            EV_SET(&k, fd, EVFILT_READ | EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
            if (kevent(q, &k, 1, nullptr, 0, nullptr) == -1)
                throw errno;
        }

        // blocks until a read is available
        void rx() {
            int nev = kevent(q, nullptr, 0, list, MAX_KEVENTS, nullptr);
            if (nev == -1)
                throw errno;

            for (int i = 0; i < nev; i++) {
                if (list[i].filter == EVFILT_READ) {
                    return;
                }
            }
        }

        // blocks until a write is available
        void tx() {
            int nev = kevent(q, nullptr, 0, list, MAX_KEVENTS, nullptr);
            if (nev == -1)
                throw errno;

            for (int i = 0; i < nev; i++) {
                if (list[i].filter == EVFILT_WRITE) {
                    return;
                }
            }
        }
    };
}