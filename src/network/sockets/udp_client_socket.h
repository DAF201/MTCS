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
#include "socket_setup.h"

using namespace std;
class cpp_udp_socket_client
{
protected:
    struct socket_pkg
    {
        unique_ptr<char[]> data;
        int size = 0;
        sockaddr_in client_addr;
    };

public:
    cpp_udp_socket_client()
    {
    }
};