#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件
*/
enum EnMsgType
{
    LOGIN_MSG = 1, // 登录消息,从 1 开始编号，后续的值自动递增（即 LOGIN_MSG_ACK = 2，LOGINOUT_MSG = 3
    LOGIN_MSG_ACK, // 登录响应消息
    LOGINOUT_MSG, // 注销消息
    REG_MSG, // 注册消息
    REG_MSG_ACK, // 注册响应消息
    ONE_CHAT_MSG, // 聊天消息
    ADD_FRIEND_MSG, // 添加好友消息

    CREATE_GROUP_MSG, // 创建群组
    ADD_GROUP_MSG, // 加入群组
    GROUP_CHAT_MSG, // 群聊天

    ASSIGNMENT_PUBLISH_MSG,   // 发布作业
    ASSIGNMENT_SUBMIT_MSG,   // 提交作业
    ASSIGNMENT_QUERY_MSG,    // 查询作业列表
    SYSTEM_BROADCAST_MSG,   //系统公告
    //FILE_TRANSFER_MSG,  // 发送文件
    //FILE_RECEIVE_MSG,    // 接收文件
    MSG_FILE_TRANSFER, // ✅ 文件传输消息
    ACK_FILE_TRANSFER,
};
// 文件传输结构
struct FileTransfer
{
    int senderid;          // 发送者 ID
    int receiverid;        // 接收者 ID
    std::string filename;  // 文件名
    std::string filedata;  // Base64 编码后的文件数据
};

#endif