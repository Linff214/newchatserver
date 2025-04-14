# test/CMakeLists.txt

add_executable(test_thread test_thread.cpp
                          ../../src/client/chatclient.cpp)

# 加入 include 路径（只需要 include 根目录）
include_directories(${PROJECT_SOURCE_DIR}/include)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
# 链接依赖库
target_link_libraries(test_thread pthread muduo_net muduo_base)

