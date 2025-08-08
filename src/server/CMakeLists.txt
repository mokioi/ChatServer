
# 把.指定目录下的所有源文件添加到SRC_LIST
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

# 生成可执行文件ChatServer, 由SRC_LIST变量所定义的源文件编译而来 
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

# ChatServer这个目标程序，需要链接muduo_net muduo_base pthread这三个库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)