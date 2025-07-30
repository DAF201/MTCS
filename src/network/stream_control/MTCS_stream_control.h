#include "../protocol/MTCS.h"
#include "../sockets/tcp_server_socket.h"
#include "../sockets/tcp_client_socket.h"
#define MAX_PACKET_LENGTH 1056
// this is the TCP controller socket, try to limite the size of data send, use the UDP sockets to transport data
class MTCS_server_controller_socket : cpp_socket_server
{
public:
    MTCS_server_controller_socket(int port) : cpp_socket_server(port)
    {
    }

    void start()
    {
        cpp_socket_server::start();
    }

    void connection_handler(SOCKET sock, char *buffer, int size) override
    {
        printf("%s\n", buffer);
        send_packet(buffer, size, sock);
    }
};

class MTCS_client_controller_socket : cpp_socket_client
{
public:
    MTCS_client_controller_socket(string server_ip, int server_port) : cpp_socket_client(server_ip, server_port)
    {
    }
};