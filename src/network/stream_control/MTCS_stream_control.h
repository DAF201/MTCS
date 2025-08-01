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
private:
public:
    MTCS_server_controller_socket(int port) : cpp_tcp_socket_server(port)
    {
    }

    void pre_connection_handler(SOCKET sock)
    {
        // first handshake
        DWORD timeout_ms = 3000; // 3 seconds timeout
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));

        unique_ptr<char[]> buffer = make_unique<char[]>(MAX_PACKET_LENGTH);
        int ret = recv(sock, buffer.get(), 8, MSG_WAITALL);
        if (ret == 0)
        {
            printf("Socket recv empty data\n");
            disconnect(sock);
            return;
        }
        if (ret == SOCKET_ERROR)
        {
            printf("Socket recv error\n");
            disconnect(sock);
            return;
        }
        if (ret < 8 || memcmp(buffer.get(), MTCS_CLIENT_HANDSHAKE_1, 8) != 0)
        {
            // disconnect if the handshake failed
            printf("Handshake data not match\n");
            disconnect(sock);
            return;
        }

        memcpy(buffer.get(), MTCS_SERVER_HANDSHAKE_1, 8);

        ret = recv(sock, buffer.get(), 8, MSG_WAITALL);
        if (ret == SOCKET_ERROR)
        {
            printf("Socket send error\n");
            disconnect(sock);
            return;
        }

        // clear the timeout after handshake
        timeout_ms = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));
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