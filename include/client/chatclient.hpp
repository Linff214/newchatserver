// chatclient.hpp
#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>

class ChatClient {
public:
    ChatClient(const std::string& ip, int port);
    bool connectToServer();
    bool register_user(const std::string& name, const std::string& password, const std::string& role);
    bool login(int id, const std::string& password);

private:
    std::string _ip;
    int _port;
    int _clientfd; // socket 文件描述符
};

#endif // CHATCLIENT_H
