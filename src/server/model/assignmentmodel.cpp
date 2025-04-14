#include "assignmentmodel.hpp"

// 插入作业
bool AssignmentModel::insert(int course_id, int teacher_id, std::string title, std::string description, std::string deadline)
{
    char sql[512];
    sprintf(sql, "INSERT INTO assignment(course_id, teacher_id, title, description, deadline) VALUES(%d, %d, '%s', '%s', '%s')",
            course_id, teacher_id, title.c_str(), description.c_str(), deadline.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 根据课程 ID 查询作业
std::vector<Assignment> AssignmentModel::queryByCourseId(int course_id)
{
    char sql[256];
    sprintf(sql, "SELECT * FROM assignment WHERE course_id = %d", course_id);

    std::vector<Assignment> assignments;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                assignments.emplace_back(atoi(row[0]), atoi(row[1]), atoi(row[2]), row[3], row[4], row[5]);
            }
            mysql_free_result(res);
        }
    }
    return assignments;
}

// 根据教师 ID 查询作业
std::vector<Assignment> AssignmentModel::queryByTeacherId(int teacher_id)
{
    char sql[256];
    sprintf(sql, "SELECT * FROM assignment WHERE teacher_id = %d", teacher_id);

    std::vector<Assignment> assignments;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                assignments.emplace_back(atoi(row[0]), atoi(row[1]), atoi(row[2]), row[3], row[4], row[5]);
            }
            mysql_free_result(res);
        }
    }
    return assignments;
}
