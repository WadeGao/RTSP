cmake_minimum_required(VERSION 3.4.1)

#项目名：Client
project(rtspServer)


# 指定根目录
set(ROOT ${PROJECT_SOURCE_DIR})

# 可执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${ROOT}/bin)

# 指定头文件路径
set(INC_DIR ${ROOT}/include)

#指定头文件目录
include_directories(${INC_DIR})

# 添加当前目录下所有的源文件
aux_source_directory(${ROOT}/src RTSP_ALL_SRCS) 

add_executable(${PROJECT_NAME} ${RTSP_ALL_SRCS})
