#include <gtest/gtest.h>

#include <a2a/examples/qwen_client.hpp>

#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

class OneShotHttpServer {
public:
    OneShotHttpServer() {
        socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd_ < 0) {
            throw std::runtime_error("failed to create test socket");
        }

        int reuse = 1;
        setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
            listen(socket_fd_, 1) < 0) {
            close(socket_fd_);
            throw std::runtime_error("failed to listen on test socket");
        }

        socklen_t address_size = sizeof(address);
        getsockname(socket_fd_, reinterpret_cast<sockaddr*>(&address), &address_size);
        port_ = ntohs(address.sin_port);

        worker_ = std::thread([this] { serve_one_request(); });
    }

    ~OneShotHttpServer() {
        if (worker_.joinable()) {
            worker_.join();
        }
        close(socket_fd_);
    }

    int port() const { return port_; }
    const std::string& request() const { return request_; }

private:
    void serve_one_request() {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(socket_fd_, &read_set);
        timeval timeout{2, 0};
        if (select(socket_fd_ + 1, &read_set, nullptr, nullptr, &timeout) <= 0) {
            return;
        }

        int client_fd = accept(socket_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            return;
        }

        std::array<char, 8192> buffer{};
        ssize_t bytes = read(client_fd, buffer.data(), buffer.size() - 1);
        if (bytes > 0) {
            request_.assign(buffer.data(), static_cast<size_t>(bytes));
        }

        const std::string body = R"({"choices":[{"message":{"content":"local-compatible-response"}}]})";
        const std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " +
            std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
        write(client_fd, response.data(), response.size());
        close(client_fd);
    }

    int socket_fd_ = -1;
    int port_ = 0;
    std::string request_;
    std::thread worker_;
};

TEST(QwenCompatibleClientTest, UsesCompatibleChatCompletionsEndpoint) {
    OneShotHttpServer server;
    const std::string base_url = "http://127.0.0.1:" + std::to_string(server.port()) + "/v1/";
    setenv("QWEN_BASE_URL", base_url.c_str(), 1);
    setenv("QWEN_MODEL", "compatible-test-model", 1);

    try {
        QwenClient client("test-key", "qwen-plus");
        EXPECT_EQ(client.chat("test system prompt", "test user message"), "local-compatible-response");
    } catch (const std::exception& error) {
        ADD_FAILURE() << error.what();
    }

    EXPECT_NE(server.request().find("POST /v1/chat/completions HTTP/1.1"), std::string::npos);
    EXPECT_NE(server.request().find("compatible-test-model"), std::string::npos);
    EXPECT_NE(server.request().find("test system prompt"), std::string::npos);
    EXPECT_NE(server.request().find("test user message"), std::string::npos);
    unsetenv("QWEN_BASE_URL");
    unsetenv("QWEN_MODEL");
}

}  // namespace
