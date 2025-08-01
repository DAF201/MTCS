#include "../protocol/MTCS.h"
#include "../sockets/tcp_server_socket_safe.h"
#include "../sockets/tcp_client_socket_safe.h"
#include "../sockets/udp_socket.h"
#define MAX_PACKET_LENGTH 1056

const char MTCS_SERVER_HANDSHAKE_1[8] = "MTCSSH1";
const char MTCS_CLIENT_HANDSHAKE_1[8] = "MTCSCH1";

// this is the TCP controller socket, try to limite the size of data send, use the UDP sockets to transport data
class MTCS_server_controller_socket : public cpp_tcp_socket_server
{
public:
    MTCS_server_controller_socket(int port) : cpp_tcp_socket_server(port)
    {
    }

    bool handshake()
    {
    }

    void start()
    {
        cpp_tcp_socket_server::start();
    }
};

class MTCS_client_controller_socket : public cpp_tcp_socket_client
{
public:
    MTCS_client_controller_socket(string server_ip, int server_port) : cpp_tcp_socket_client(server_ip, server_port)
    {
    }
};

// this is the UDP socket, try to use this to transport data, it is faster, conenction less, and cost less resources
class MTCS_transportation_socket : public cpp_udp_socket
{
public:
    MTCS_transportation_socket(int port = 0) : cpp_udp_socket(port)
    {
    }
};