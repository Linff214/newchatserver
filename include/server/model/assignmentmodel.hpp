#ifndef ASSIGNMENTMODEL_H
#define ASSIGNMENTMODEL_H

#include "db.h"
#include <vector>
// 作业类
class Assignment
{
public:
    Assignment(int id, int course_id, int teacher_id, std::string title, std::string description, std::string deadline)
        : id(id), course_id(course_id), teacher_id(teacher_id), title(title), description(description), deadline(deadline) {}

    int getId() const { return id; }
    int getCourseId() const { return course_id; }
    int getTeacherId() const { return teacher_id; }
    std::string getTitle() const { return title; }
    std::string getDescription() const { return description; }
    std::string getDeadline() const { return deadline; }

private:
    int id;
    int course_id;
    int teacher_id;
    std::string title;
    std::string description;
    std::string deadline;
};

// 作业数据库操作类
class AssignmentModel
{
public:
    bool insert(int course_id, int teacher_id, std::string title, std::string description, std::string deadline);
    std::vector<Assignment> queryByCourseId(int course_id);
    std::vector<Assignment> queryByTeacherId(int teacher_id);
};

#endif


