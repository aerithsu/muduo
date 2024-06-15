#include <iostream>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <functional>
#include <thread>

using namespace std;
using namespace muduo::net;
using namespace muduo;
// 把网络IO和用户的业务分开
// 用户的连接和断开,用户的可读写时间


//1.组合TcpServer对象
//2.创建EventLoop事件循环对象的指针
//3.明确TcpServer构造函数的参数,输出ChatServer的构造函数
//4.在当前服务器的类的构造函数中,注册处理连接的回调函数和处理读写数据的回调函数
//5.设置合适的服务端线程数量,muduo会自己分配IO线程和worker线程
class ChatServer {
public:
    ChatServer(EventLoop *loop, //事件循环
               const InetAddress &listenAddr, //Ip + Port
               const string &nameArg) //服务器的名字
            : _server(loop, listenAddr, nameArg), _loop(loop) {
        //给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback([this](const TcpConnectionPtr &ptr) {
            onConnection(ptr);
        });
        //给服务器注册用户读写事件回调
        _server.setMessageCallback(
                [this](auto &&PH1, auto &&PH2, auto &&PH3) {
                    onMessage(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                              std::forward<decltype(PH3)>(PH3));
                });
        //设置服务器端的线程数量,1个IO线程,3个Worker线程
        _server.setThreadNum(4);
    }

    //开启事件循环
    void start() {
        _server.start();
    }

private:
    //专门处理用户的连接的创建和断开的回调
    void onConnection(const TcpConnectionPtr &conn) {
        if (conn->connected())
            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << " state online" << endl;
        else {
            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << " state outline"
                 << endl;
            conn->shutdown();
        }


    }

    //处理用户的读写事件的回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time) {
        string buffer = buf->retrieveAllAsString();
        cout << "recv data:" << buffer << " time:" << time.toString() << endl;
        conn->send(buf);
        //_loop->quit(); //不为
    }


    TcpServer _server;
    EventLoop *_loop;
};

int main(int argc, char const *argv[]) {
    EventLoop loop; //类似于创建了一个epoll
    InetAddress addr{"127.0.0.1", 7999};
    ChatServer server(&loop, addr, "ChatServer");
    server.start(); //listenfd epoll_ctl => epoll
    loop.loop(); //以阻塞方式等待新用户连接新用户连接,已连接用户的读写事件等.
    return 0;
}


// 0 errornum = EAGAIN 正常返回,一般会继续调用
// -1 连接终端
// 异步阻塞不合理
// 都是异步非阻塞,异步调用后,原线程可以继续执行