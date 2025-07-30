#include "../protocol/MTCS.h"
#include "../sockets/tcp_server_socket.h"
#define MAX_PACKET_LENGTH 1056
// this is the TCP controller socket, try to limite the size of data send, use the UDP sockets to transport data
class MTCS_controller_socket : cpp_socket_server
{
    MTCS_controller_socket(int port) : cpp_socket_server(port)
    {
    }

    // connection handler implementation
    void connection_handler(SOCKET sock)
    {
        char buffer[MAX_PACKET_LENGTH];
        while (true)
        {
            
        }
    }
};