#include <future>
#include <iostream>
#include "tcp.cc"

void handle_client(tcp::conn conn) {
    // Echo whatever the client sends
    while (true) {
        auto result = conn.Read();
        conn.Write(result);
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
}