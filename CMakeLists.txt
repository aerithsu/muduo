cmake_minimum_required(VERSION 3.22)
project(muduo)

set(CMAKE_CXX_STANDARD 17)
#设置最终编译为的.so的动态库的路径
set(LIBRARY_OUTPUT_PATH  ${PROJECT_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fPIC")

find_package(Threads REQUIRED)
# 定义参与编译的源码文件
#aux_source_directory(. SRC_LIST)
#编译生成动态库muduo
add_library(muduo SHARED
        Noncopyable.h
        Logger.cpp
        Logger.h
        Timestamp.h
        Timestamp.cpp
        Noncopyable.cpp
        InetAddress.cpp
        InetAddress.h
        Channel.cpp
        Channel.h
        TcpServer.cpp
        TcpServer.h
        EventLoop.cpp
        EventLoop.h
        Poller.cpp
        Poller.h
        DefaultPoller.cpp
        EpollPoller.cpp
        EpollPoller.h
        CurrentThread.cpp
        CurrentThread.h
        EventLoopThreadPool.cpp
        EventLoopThreadPool.h
        Thread.cpp
        Thread.h
        EventLoopThread.cpp
        EventLoopThread.h
        Acceptor.cpp
        Acceptor.h
        Socket.cpp
        Socket.h
        TcpConnection.cpp
        TcpConnection.h
        Buffer.cpp
        Buffer.h
        Callbacks.h)
#muduo_base依赖muduo_net,所有muduo_net要写在前面
target_link_libraries(muduo PRIVATE muduo_net muduo_base Threads::Threads)
