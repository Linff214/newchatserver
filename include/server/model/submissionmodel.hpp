#ifndef SUBMISSIONMODEL_HPP
#define SUBMISSIONMODEL_HPP

#include "db.h"
#include <vector>

// 提交作业类
class Submission
{
public:
    Submission(int id, int assignment_id, int student_id, std::string file_path, int score, std::string submit_time)
        : id(id), assignment_id(assignment_id), student_id(student_id), file_path(file_path), score(score), submit_time(submit_time) {}

    int getId() const { return id; }
    int getAssignmentId() const { return assignment_id; }
    int getStudentId() const { return student_id; }
    std::string getFilePath() const { return file_path; }
    int getScore() const { return score; }
    std::string getSubmitTime() const { return submit_time; }

private:
    int id;
    int assignment_id;
    int student_id;
    std::string file_path;
    int score;
    std::string submit_time;
};

// 提交作业数据库操作类
class SubmissionModel
{
public:
    bool insert(int assignment_id, int student_id, std::string file_path);
    std::vector<Submission> queryByAssignmentId(int assignment_id);
    bool gradeSubmission(int submission_id, int score);
};

#endif
