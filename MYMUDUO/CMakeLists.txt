    # cmake版本2.9以上
    cmake_minimum_required(VERSION 2.9)
    # 项目文件
    project(MYMUDUO)

    # 编译MYMUDUO设置动态库的路径,根目录lib文件夹下
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
    # 设置调试信，启动c++11
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

    # 定义参与编译的源文件
    aux_source_directory(. SRC_LIST)
    # 编译生成动态库
    add_library(mymuduo SHARED ${SRC_LIST})  