#include "chatservice.hpp"
#include "public.hpp"
#include <fstream>
#include <iostream>
#include <mysql/mysql.h> 
#include <muduo/base/Logging.h>
#include <vector>
#include "base64.hpp"
using namespace std;
using namespace muduo;
static string server = "127.0.0.1";
static string user = "root";
static string password = "214911_lXX";
static string dbname = "newchat";
// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ASSIGNMENT_PUBLISH_MSG, std::bind(&ChatService::publish_assignment, this, _1, _2, _3)});
    _msgHandlerMap.insert({ASSIGNMENT_SUBMIT_MSG, std::bind(&ChatService::submit_assignment, this, _1, _2, _3)});
    _msgHandlerMap.insert({SYSTEM_BROADCAST_MSG, std::bind(&ChatService::send_system_announcement, this, _1, _2, _3)});
    _msgHandlerMap.insert({MSG_FILE_TRANSFER, std::bind(&ChatService::fileTransfer, this, _1, _2, _3)});
    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        if (_redis.subscribe("system_broadcast"))
        {
            cout << "Subscribed to system_broadcast successfully!" << endl;
        }
        else
        {
            cerr << "Failed to subscribe to system_broadcast!" << endl;
        }
        _redis.subscribe("system_broadcast");
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
        
    }
    _conn = mysql_init(nullptr);
    if (_conn == nullptr)
    {
        cerr << "MySQL initialization failed!" << endl;
        exit(EXIT_FAILURE);
    }

    // 连接 MySQL 数据库
    if (!mysql_real_connect(_conn, server.c_str(), user.c_str(),password.c_str(), dbname.c_str(), 3306, nullptr, 0))
    {
        cerr << "MySQL connection error: " << mysql_error(_conn) << endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "MySQL connection successful!" << endl;
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把online状态的用户，设置成offline
    _userModel.resetState();
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务  id  pwd   pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id); 
            _redis.subscribe("system_broadcast");
            // 登录成功，更新用户状态信息 state offline=>online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在，用户存在但是密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}

// 处理注册业务  name  password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];
    string role = js.contains("role") ? js["role"] : "student";  // 默认 student
    User user;
    user.setName(name);
    user.setPwd(pwd);
    user.setRole(role);  // 添加 role 字段
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid); 

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId()); 

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息   服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线 
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}
//教师发布作业业务
void ChatService::publish_assignment(const TcpConnectionPtr& conn, json& js, Timestamp)
{
    int teacher_id = js["id"];
    int course_id = js["courseid"];
    std::string title = js["title"];
    std::string description = js["description"];
    std::string deadline = js["deadline"];

    int assignment_id = _assignmentModel.insert(course_id, teacher_id, title, description, deadline);
    
    json response;
    response["msgid"] = ASSIGNMENT_PUBLISH_MSG;
    response["assignmentid"] = assignment_id;
    response["errno"] = (assignment_id == -1) ? 1 : 0;
    conn->send(response.dump());
}
// 学生提交作业
void ChatService::submit_assignment(const TcpConnectionPtr& conn, json& js, Timestamp)
{
    int assignment_id = js["assignmentid"];
    int student_id = js["id"];
    std::string file_path = js["file_path"];  // 存储路径

    // 1. 验证 student_id 是否存在且为学生
    string check_user_sql = "SELECT COUNT(*) FROM user WHERE id = " + to_string(student_id) + " AND role = 'student'";
    if (mysql_query(_conn, check_user_sql.c_str()))
    {
        cerr << "Error checking student_id: " << mysql_error(_conn) << endl;
        return;
    }
    MYSQL_RES *res_user = mysql_store_result(_conn);
    MYSQL_ROW row_user = mysql_fetch_row(res_user);
    if (atoi(row_user[0]) == 0)
    {
        // student_id 无效
        json response;
        response["msgid"] = ASSIGNMENT_SUBMIT_MSG;
        response["errno"] = 1;
        response["errmsg"] = "Student ID is invalid.";
        conn->send(response.dump());
        return;
    }
    mysql_free_result(res_user);

    // 2. 验证 assignment_id 是否存在
    string check_assignment_sql = "SELECT COUNT(*) FROM assignment WHERE id = " + to_string(assignment_id);
    if (mysql_query(_conn, check_assignment_sql.c_str()))
    {
        cerr << "Error checking assignment_id: " << mysql_error(_conn) << endl;
        return;
    }
    MYSQL_RES *res_assignment = mysql_store_result(_conn);
    MYSQL_ROW row_assignment = mysql_fetch_row(res_assignment);
    if (atoi(row_assignment[0]) == 0)
    {
        // assignment_id 无效
        json response;
        response["msgid"] = ASSIGNMENT_SUBMIT_MSG;
        response["errno"] = 1;
        response["errmsg"] = "Assignment does not exist.";
        conn->send(response.dump());
        mysql_free_result(res_assignment);
        return;
    }
    mysql_free_result(res_assignment);

    // 3. 验证 student_id 是否是 math_class 群的成员
    string check_group_sql = "SELECT COUNT(*) FROM groupuser WHERE userid = " + to_string(student_id) + " AND groupid = " + to_string(1); // 假设 math_class 的 groupid 为 1
    if (mysql_query(_conn, check_group_sql.c_str()))
    {
        cerr << "Error checking group membership: " << mysql_error(_conn) << endl;
        return;
    }
    MYSQL_RES *res_group = mysql_store_result(_conn);
    MYSQL_ROW row_group = mysql_fetch_row(res_group);
    if (atoi(row_group[0]) == 0)
    {
        // 用户不是群成员
        json response;
        response["msgid"] = ASSIGNMENT_SUBMIT_MSG;
        response["errno"] = 1;
        response["errmsg"] = "User is not a member of the group.";
        conn->send(response.dump());
        mysql_free_result(res_group);
        return;
    }
    mysql_free_result(res_group);

    // 4. 检查是否已经提交过作业（防止重复提交）
    string check_submission_sql = "SELECT COUNT(*) FROM submission WHERE assignment_id = " + to_string(assignment_id) + " AND student_id = " + to_string(student_id);
    if (mysql_query(_conn, check_submission_sql.c_str()))
    {
        cerr << "Error checking submission: " << mysql_error(_conn) << endl;
        return;
    }
    MYSQL_RES *res_submission = mysql_store_result(_conn);
    MYSQL_ROW row_submission = mysql_fetch_row(res_submission);
    if (atoi(row_submission[0]) > 0)
    {
        // 已经提交过作业
        json response;
        response["msgid"] = ASSIGNMENT_SUBMIT_MSG;
        response["errno"] = 1;
        response["errmsg"] = "Duplicate submission. Assignment already submitted.";
        conn->send(response.dump());
        mysql_free_result(res_submission);
        return;
    }
    mysql_free_result(res_submission);

    // 5. 插入作业提交记录
    bool state = _submissionModel.insert(assignment_id, student_id, file_path);

    // 6. 发送提交成功的响应
    json response;
    response["msgid"] = ASSIGNMENT_SUBMIT_MSG;
    if (state)
    {
        response["errno"] = 0;
    }
    else
    {
        response["errno"] = 1;
        response["errmsg"] = "Submission failed due to internal error.";
    }
    conn->send(response.dump());
}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线 
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}
// 教务通知系统
void ChatService::send_system_announcement(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    std::string target = js["target"];
    std::string content = js["content"];

    // 1. 验证 target 是否有效
    if (target != "all" && target != "teachers" && target != "students")
    {
        json response;
        response["msgid"] = SYSTEM_BROADCAST_MSG;
        response["errno"] = 1;
        response["errmsg"] = "Invalid target. Only 'all', 'teachers', or 'students' are allowed.";
        conn->send(response.dump());
        return;
    }

    // 2. 构建 SQL 语句，将通知插入到 notifications 表
    string insert_sql = "INSERT INTO notifications(target, content) VALUES('" +
                        target + "', '" + content + "')";

    if (mysql_query(_conn, insert_sql.c_str()))
    {
        cerr << "Error inserting notification: " << mysql_error(_conn) << endl;

        // 3. 插入失败，返回错误信息
        json response;
        response["msgid"] = SYSTEM_BROADCAST_MSG;
        response["errno"] = 1;
        response["errmsg"] = "Failed to insert notification.";
        conn->send(response.dump());
        return;
    }

    // 4. Redis 广播通知，确保 JSON 格式规范
    json message;
    message["msgid"] = SYSTEM_BROADCAST_MSG;
    message["target"] = target;
    message["content"] = content;

    // ✅ 确保使用 .dump() 将 JSON 格式化为字符串
    string broadcast_msg = message.dump();
    _redis.publish("system_broadcast", broadcast_msg);

    // 5. 通知插入成功，返回成功信息
    json response;
    response["msgid"] = SYSTEM_BROADCAST_MSG;
    response["errno"] = 0;
    response["msg"] = "Notification sent and stored successfully.";
    conn->send(response.dump());
}

// 处理文件传输
void ChatService::fileTransfer(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int senderid = js["senderid"];
    int receiverid = js["receiverid"];
    string filename = js["filename"];
    string filedata = js["filedata"];

    // 文件保存路径
    string filepath = "./received_files/" + filename;
    ofstream outfile(filepath, ios::binary);
    if (!outfile)
    {
        LOG_ERROR << "Failed to create file: " << filepath;
        return;
    }

    // 解码 Base64
    string decodedData = base64_decode(filedata);

    // 写入文件
    outfile.write(decodedData.c_str(), decodedData.size());
    outfile.close();

    LOG_INFO << "File received and saved to: " << filepath;

    // 发送确认消息
    json ackMsg;
    ackMsg["msgid"] = MSG_FILE_TRANSFER;
    ackMsg["result"] = "File received successfully.";
    conn->send(ackMsg.dump());
}
void ChatService::handleRedisSubscribeMessage(int userid_or_channel, string msg)
{
    cout << ">>> Received message from Redis channel: " << userid_or_channel << ", msg: " << msg << endl;

    // ✅ 如果是广播消息，userid_or_channel == 0
    if (userid_or_channel == 0)
    {
        // 解析 JSON 数据
        try
        {
            json js = json::parse(msg);
            if (js.contains("target") && js.contains("content"))
            {
                string target = js["target"];
                string content = js["content"];
                cout << ">>> Broadcast target: " << target << ", content: " << content << endl;

                lock_guard<mutex> lock(_connMutex);
                cout << "User connections size: " << _userConnMap.size() << endl;

                for (auto &pair : _userConnMap)
                {
                    int uid = pair.first;
                    User user = _userModel.query(uid);  // 查询用户信息

                    cout << "Checking user [" << uid << "] with role: " << user.getRole() << endl;

                    // ✅ 发送给所有用户
                    if (target == "all")
                    {
                        cout << ">>> Sending to user [" << uid << "]" << endl;
                        pair.second->send(msg);
                    }
                    // ✅ 发送给教师
                    else if (target == "teachers" && user.getRole() == "teacher")
                    {
                        cout << ">>> Sending to teacher [" << uid << "]" << endl;
                        pair.second->send(msg);
                    }
                    // ✅ 发送给学生
                    else if (target == "students" && user.getRole() == "student")
                    {
                        cout << ">>> Sending to student [" << uid << "]" << endl;
                        pair.second->send(msg);
                    }
                }
            }
        }
        catch (exception &e)
        {
            cerr << ">>> Error parsing broadcast message: " << e.what() << endl;
        }
    }
    else
    {
        // ✅ 如果是点对点通信
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid_or_channel);
        if (it != _userConnMap.end())
        {
            cout << ">>> Sending point-to-point message to user [" << userid_or_channel << "]" << endl;
            it->second->send(msg);
        }
        else
        {
            cout << ">>> Saving message to offline storage for user [" << userid_or_channel << "]" << endl;
            _offlineMsgModel.insert(userid_or_channel, msg);
        }
    }
}




