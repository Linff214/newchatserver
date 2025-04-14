#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

#include "client/chatclient.hpp"


std::atomic<int> total_success{0};
std::atomic<int> total_fail{0};
std::atomic<long> total_response_ms{0};

void simulate_client(int id) {
    // 计时
    auto start = std::chrono::steady_clock::now();

    ChatClient client("127.0.0.1", 6000);
    if (!client.connectToServer()) {
        std::cerr << "❌ Client " << id << " failed to connect." << std::endl;
        total_fail++;
        return;
    }

    client.register_user("user" + std::to_string(id), "123456", "student");
    client.login(id, "123456");

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    total_success++;
    total_response_ms += duration.count();

    std::cout << "Client " << id << " response time: " << duration.count() << " ms" << std::endl;
}

int main() {
    const int CLIENT_NUM = 2000;
    std::vector<std::thread> clients;

    for (int i = 1; i <= CLIENT_NUM; ++i) {
        clients.emplace_back(simulate_client, i);
    }

    for (auto& t : clients) t.join();

    std::cout << "✅ 成功登录客户端数： " << total_success << std::endl;
    std::cout << "❌ 失败登录客户端数： " << total_fail << std::endl;
    std::cout << "📊 平均响应时间： " 
              << (CLIENT_NUM > 0 ? total_response_ms / CLIENT_NUM : 0)
              << " ms" << std::endl;
    return 0;
}
