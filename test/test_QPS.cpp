#include <iostream>
#include <chrono>
#include <mysql/mysql.h>  // C API

using namespace std;
using namespace std::chrono;
static string server = "127.0.0.1";
static string user = "root";
static string password = "214911_lXX";
static string dbname = "newchat";
int main() {
    MYSQL *conn = mysql_init(nullptr);
    mysql_real_connect(conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    
    const int QUERY_TIMES = 1000;
    const char* sql = "SELECT * FROM users WHERE id = 1";  // 示例查询

    auto start = steady_clock::now();
    for (int i = 0; i < QUERY_TIMES; ++i) {
        mysql_query(conn, sql);
        MYSQL_RES* result = mysql_store_result(conn);
        mysql_free_result(result);
    }
    auto end = steady_clock::now();

    auto duration = duration_cast<milliseconds>(end - start).count();
    double qps = (QUERY_TIMES * 1000.0) / duration;

    cout << "Total queries: " << QUERY_TIMES << endl;
    cout << "Total time: " << duration << " ms" << endl;
    cout << "QPS: " << qps << " queries/second" << endl;

    mysql_close(conn);
    return 0;
}
