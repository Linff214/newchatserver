// chatclient.cpp
#include "client/chatclient.hpp"
#include "json.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

using json = nlohmann::json;
using namespace std;

ChatClient::ChatClient(const string& ip, int port)
    : _ip(ip), _port(port), _clientfd(-1) {}

bool ChatClient::connectToServer() {
    _clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_clientfd == -1) {
        cerr << "socket create error" << endl;
        return false;
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(_port);
    server.sin_addr.s_addr = inet_addr(_ip.c_str());

    if (connect(_clientfd, (sockaddr*)&server, sizeof(server)) == -1) {
        cerr << "connect error" << endl;
        return false;
    }

    return true;
}

bool ChatClient::register_user(const string& name, const string& password, const string& role) {
    json js;
    js["msgid"] = 4;
    js["name"] = name;
    js["password"] = password;
    js["role"] = role;
    string request = js.dump();

    if (send(_clientfd, request.c_str(), request.size() + 1, 0) == -1) {
        cerr << "send error" << endl;
        return false;
    }

    char buffer[1024] = {0};
    int len = recv(_clientfd, buffer, sizeof(buffer), 0);
    if (len <= 0) {
        cerr << "recv error" << endl;
        return false;
    }

    json response = json::parse(buffer);
    return response["errno"].get<int>() == 0;
}

bool ChatClient::login(int id, const string& password) {
    json js;
    js["msgid"] = 1;
    js["id"] = id;
    js["password"] = password;
    string request = js.dump();

    if (send(_clientfd, request.c_str(), request.size() + 1, 0) == -1) {
        cerr << "send error" << endl;
        return false;
    }

    char buffer[1024] = {0};
    int len = recv(_clientfd, buffer, sizeof(buffer), 0);
    if (len <= 0) {
        cerr << "recv error" << endl;
        return false;
    }

    json response = json::parse(buffer);
    return response["errno"].get<int>() == 0;
}
