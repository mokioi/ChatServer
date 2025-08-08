# 把.指定目录下的所有源文件添加到SRC_LIST
aux_source_directory(. SRC_LIST)

# 生成可执行文件ChatClient, 由SRC_LIST变量所定义的源文件编译而来 
add_executable(ChatClient ${SRC_LIST})

# ChatServer这个目标程序，需要链接pthread这个库文件
target_link_libraries(ChatClient pthread)