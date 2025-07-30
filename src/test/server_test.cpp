#include "../network/sockets/tcp_server_socket.h"
using namespace std;
class test_server : cpp_socket_server
{
public:
    test_server(int port) : cpp_socket_server(port) {};
};
int main()
{
    auto s = cpp_socket_server(1024);
    s.start();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
