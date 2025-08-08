# 配置头文件搜索路径
# include_directories(/usr/local/include)
# 配置库文件搜索路径
# link_directories(/usr/local/lib)

# 设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)

# 设置可执行文件最终存储路径，PROJECT_SOURCE_DIR是项目的源代码根目录，即CMakeLists.txt所在目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 把.指定目录下的所有源文件添加到SRC_LIST
# aux_source_directory(. SRC_LIST)

# 生成可执行文件server, 由SRC_LIST变量所定义的源文件编译而来 
add_executable(server ${SRC_LIST})   

# server这个目标程序，需要链接muduo_net muduo_base pthread这三个库文件
target_link_libraries(server muduo_net muduo_base pthread)