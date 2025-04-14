USE newchat;

-- 用户表
CREATE TABLE IF NOT EXISTS user (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(100) NOT NULL,
    role ENUM('student', 'teacher', 'admin') NOT NULL,
    state ENUM('online', 'offline') DEFAULT 'offline'
);

-- 好友关系表
CREATE TABLE IF NOT EXISTS friend (
    user_id INT NOT NULL,
    friend_id INT NOT NULL,
    PRIMARY KEY (user_id, friend_id),
    FOREIGN KEY (user_id) REFERENCES user(id),
    FOREIGN KEY (friend_id) REFERENCES user(id)
);

-- 创建 allgroup 表
CREATE TABLE allgroup (
    id INT PRIMARY KEY AUTO_INCREMENT COMMENT '组id',
    groupname VARCHAR(50) NOT NULL UNIQUE COMMENT '组名称',
    groupdesc VARCHAR(200) DEFAULT '' COMMENT '组功能描述'
);

-- 创建 groupuser 表
CREATE TABLE groupuser (
    groupid INT NOT NULL COMMENT '组id',
    userid INT NOT NULL COMMENT '组员id',
    grouprole ENUM('creator', 'normal') DEFAULT 'normal' COMMENT '组内角色',
    PRIMARY KEY (groupid, userid)
);


-- 作业发布表
CREATE TABLE IF NOT EXISTS assignment (
    id INT PRIMARY KEY AUTO_INCREMENT,
    course_id INT NOT NULL,
    teacher_id INT NOT NULL,
    title VARCHAR(200) NOT NULL,
    description TEXT NOT NULL,
    deadline DATETIME NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 作业提交表
CREATE TABLE IF NOT EXISTS submission (
    id INT PRIMARY KEY AUTO_INCREMENT,
    assignment_id INT NOT NULL,
    student_id INT NOT NULL,
    file_path VARCHAR(255) NOT NULL,
    score INT DEFAULT NULL,
    submit_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 教务通知表
CREATE TABLE IF NOT EXISTS notifications (
    id INT PRIMARY KEY AUTO_INCREMENT,
    target ENUM('all', 'teachers', 'students') NOT NULL,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 离线消息表
CREATE TABLE IF NOT EXISTS offlinemessage (
    id INT PRIMARY KEY AUTO_INCREMENT,
    receiver_id INT NOT NULL,
    message TEXT NOT NULL,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 文件传输表
CREATE TABLE IF NOT EXISTS file_transfers (
    id INT PRIMARY KEY AUTO_INCREMENT,
    sender_id INT NOT NULL,
    receiver_id INT NOT NULL,
    file_name VARCHAR(255) NOT NULL,
    file_path VARCHAR(255) NOT NULL,
    file_size BIGINT NOT NULL,                 -- ✅ 新增：存储文件大小
    file_type VARCHAR(50),                     -- ✅ 新增：存储文件类型（pdf, jpg, ppt 等）
    transfer_status ENUM('success', 'fail', 'pending') DEFAULT 'pending',  -- ✅ 新增：文件状态
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    -- 外键关联用户
    FOREIGN KEY (sender_id) REFERENCES user(id) ON DELETE CASCADE,
    FOREIGN KEY (receiver_id) REFERENCES user(id) ON DELETE CASCADE
);
-- 创建索引优化查询
CREATE INDEX idx_sender ON file_transfers(sender_id);
CREATE INDEX idx_receiver ON file_transfers(receiver_id);
