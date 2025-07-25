#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

class cpp_socket_server
{

protected:
    struct recv_pkg
    {
        char *data = nullptr;
        int size = 0;
    };
    struct send_pkg
    {
        char *data = nullptr;
        int size = 0;
    };

    atomic<bool> stop_flag;
    WORD sock_version;
    WSADATA WSA_data;

    thread RECV_THREAD;
    thread SEND_THREAD;

    queue<send_pkg> send_queue;
    queue<recv_pkg> recv_queue;

    mutex send_mutex;
    condition_variable send_cv;

    mutex recv_mutex;
    condition_variable recv_cv;

public:
    cpp_socket_server(const int server_port) : stop_flag(false)
    {
        sock_version = MAKEWORD(2, 2);
    }
};