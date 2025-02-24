
# 一般项目下（标准）：
# bin: 可执行文件
# lib:
# include: 头文件
# src:
# build: 项目建立文件
# example:
# thridparty:
# CMakeLists.txt
# autobuild.sh

# 通过设置cmake 能够省掉，手打执行命令：g++ -o server -muduo_server.cpp -lmuduo_net -lmuduo_base -lpthread


# ----------- 指导 编译 testmuduo 文件夹 --------------------

# 配置头文件搜索路径
# include _directories()

# 配置库文件搜索路径
# link_directories()

# 设置需要编译的源文件列表
set(SRC_LITST ./muduo_server.cpp)


# 设置可执行文件[bin] 最终存储的路径
# PROJECT_SOURCE_DIR ：工程的根目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 把.指定路径下的所有源文件名字放入变量名SRC_LITST 里面
# aux_source_directory(.SRC_LITST)

# 表示生成的可执行文件，由 SRC_LITST 变量所定义的源文件编译而来
add_executable(server ${SRC_LITST})

# 编译路径 会自动补全 - 
# 表示 server 这个目标程序，需要连接 这三个库文件
target_link_libraries(server muduo_net muduo_base pthread)

