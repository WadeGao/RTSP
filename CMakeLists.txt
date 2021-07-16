cmake_minimum_required(VERSION 3.4.1)

#项目名：Client
project(rtspServer)

set (CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE "Release" )

set(ROOT ${PROJECT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${ROOT}/bin)

# 指定头文件目录
include_directories(${ROOT}/include)

#生成动态库.so文件，默认在lib文件夹中
add_library(rtp SHARED ${ROOT}/src/rtp.cc)
add_library(H264 SHARED ${ROOT}/src/H264.cc)
#add_library(rtsp SHARED ${ROOT}/src/rtsp.cc)

#添加可执行程序
#add_executable(${PROJECT_NAME} ${ROOT}/src/main.cc)

#连接，将动态库链接到可执行文件
#target_link_libraries(${PROJECT_NAME} rtp H264)