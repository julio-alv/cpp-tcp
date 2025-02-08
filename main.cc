#include <future>
#include "tcp.cc"

void handle_client(tcp::Conn conn) {
    while (true) {
        auto result = conn.Read();
        if (!result) {
            perror("Client disconnected or error");
            close(conn.fd);
            return;
        }
        std::print("Received: {}", result.value());

        conn.Write("Hello!\n");
    }
}

int main() {
    auto create_listener = tcp::Listen(8080);
    if (!create_listener) {
        perror("Failed to listen");
        return 1;
    }

    auto listener = create_listener.value();
    std::println("Listening on port 8080...");

    std::vector<std::future<void>> threads;

    while (true) {
        auto create_conn = listener.Accept();
        if (!create_conn) {
            perror("Failed to accept");
            continue;
        }

        std::println("Client Connected");
        auto conn = create_conn.value();

        threads.push_back(std::async(std::launch::async, handle_client, std::move(conn)));
    }

    return 0;
}