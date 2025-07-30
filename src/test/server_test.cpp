#include "../network/sockets/tcp_server_socket.h"
using namespace std;
class test_server : cpp_socket_server
{
public:
    test_server(int port) : cpp_socket_server(port) {};

    char buffer[1024];

private:
    void connection_handler_func(SOCKET sock)
    {
        char buffer[1024];
        while (true)
        {
            int received = recv(sock, buffer, sizeof(buffer), 0);

            if (received == 0)
            {
                printf("Connection closed by peer\n");
                return;
            }
            else if (received == SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK || err == WSAEINTR)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    return;
                }
                printf("Recv error: %d\n", err);
                return;
            }

            char *data_copy = new char[received];
            memcpy(data_copy, buffer, received);

            printf("Received: %.*s\n", received, data_copy); // safer print

            {
                std::lock_guard<std::mutex> lock(recv_mutex);
                recv_queue.push(socket_pkg{data_copy, received, sock});
            }
        }
    }
};
int main()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
