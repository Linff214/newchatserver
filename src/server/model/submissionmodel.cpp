#include "submissionmodel.hpp"

// 学生提交作业
bool SubmissionModel::insert(int assignment_id, int student_id, std::string file_path)
{
    char sql[512];
    sprintf(sql, "INSERT INTO submission(assignment_id, student_id, file_path) VALUES(%d, %d, '%s')",
            assignment_id, student_id, file_path.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        return mysql.update(sql);
    }
    return false;
}

// 查询某个作业的提交记录
std::vector<Submission> SubmissionModel::queryByAssignmentId(int assignment_id)
{
    char sql[256];
    sprintf(sql, "SELECT * FROM submission WHERE assignment_id = %d", assignment_id);

    std::vector<Submission> submissions;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                submissions.emplace_back(atoi(row[0]), atoi(row[1]), atoi(row[2]), row[3], atoi(row[4]), row[5]);
            }
            mysql_free_result(res);
        }
    }
    return submissions;
}

// 教师评分
bool SubmissionModel::gradeSubmission(int submission_id, int score)
{
    char sql[256];
    sprintf(sql, "UPDATE submission SET score = %d WHERE id = %d", score, submission_id);

    MySQL mysql;
    if (mysql.connect())
    {
        return mysql.update(sql);
    }
    return false;
}
