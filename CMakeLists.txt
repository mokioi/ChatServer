cmake_minimum_required(VERSION 3.0) # CMake 最低版本号
project(ChatServer)  # 项目名称

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g) # -g 表示开启调试

# 设置可执行文件最终存储路径，PROJECT_SOURCE_DIR是项目的源代码根目录，即CMakeLists.txt所在目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)


# 指定搜索的子目录
add_subdirectory(src)