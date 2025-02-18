#include <future>
#include "tcp.cc"
#include <iostream>

void handle_client(tcp::conn conn) {
    conn.Write("Welcome to the TCP Server!\n");
    while (true) {
        auto result = conn.Read();
        std::cout << "Received: " << result;
        conn.Write("Hello!\n");
    }
}

int main() {
    auto listener = tcp::Listen(8080);
    std::cout << "Listening on port 8080..." << std::endl;

    std::vector<std::future<void>> threads;

    while (true) {
        auto conn = listener.Accept();
        threads.push_back(std::async(std::launch::async, handle_client, std::move(conn)));
    }

    return 0;
}