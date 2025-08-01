#include "../protocol/MTCS.h"
#include "../sockets/tcp_server_socket_safe.h"
#include "../sockets/tcp_client_socket_safe.h"
#include "../sockets/udp_socket.h"
#include <random>
#define MAX_PACKET_LENGTH 1056
using namespace std;
const char MTCS_SERVER_HANDSHAKE_1[8] = "MTCSSH1";
const char MTCS_CLIENT_HANDSHAKE_1[8] = "MTCSCH1";

// this is the TCP controller socket, try to limite the size of data send, use the UDP sockets to transport data
class MTCS_server_controller_socket : public cpp_tcp_socket_server
{

private:
    static vector<int> stream_id;
    // save the UDP sockets of server and client address to send to
    vector<MTCS_transportation_socket> server_UDP_sockets;
    vector<sockaddr_in> client_UDP_addresses;

public:
    MTCS_server_controller_socket(int port) : cpp_tcp_socket_server(port) {}

    void pre_connection_handler(SOCKET sock)
    {
        // first handshake, confirm the client can send and understand the handshake
        // 3 seconds timeout
        DWORD timeout_ms = 3000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));

        unique_ptr<char[]> buffer = make_unique<char[]>(MAX_PACKET_LENGTH);
        int ret = recv(sock, buffer.get(), 8, MSG_WAITALL);
        if (ret == 0)
        {
            printf("First Handshake recv empty data\n");
            disconnect(sock);
            return;
        }
        if (ret == SOCKET_ERROR)
        {
            printf("First Handshake recv error\n");
            disconnect(sock);
            return;
        }
        if (ret < 8 || memcmp(buffer.get(), MTCS_CLIENT_HANDSHAKE_1, 8) != 0)
        {
            // disconnect if the handshake failed
            printf("First Handshake data not match\n");
            disconnect(sock);
            return;
        }

        memcpy(buffer.get(), MTCS_SERVER_HANDSHAKE_1, 8);

        ret = recv(sock, buffer.get(), 8, MSG_WAITALL);
        if (ret == SOCKET_ERROR)
        {
            printf("First Handshake send error\n");
            disconnect(sock);
            return;
        }

        // end of the first handshake

        // second handshake, check the ports with clients (create 8 udp ports here)

        // create UDP sockets, and get ports to client
        int ports_buffer[8];
        for (int i = 0; i < 8; i++)
        {
            MTCS_transportation_socket s = MTCS_transportation_socket();
            server_UDP_sockets.push_back(s);
            ports_buffer[i] = s.get_port();
        }

        // send the ports to client, then wait for client to reply for the ports going to use
        ret = send(sock, (char *)ports_buffer, sizeof(ports_buffer), 0);
        if (ret == SOCKET_ERROR)
        {
            printf("Second Handshake send error\n");
            disconnect(sock);
            return;
        }

        // recv the client's response about which ports it is going to use
        ret = recv(sock, (char *)ports_buffer, sizeof(ports_buffer), 0);
        if (ret != sizeof(ports_buffer))
        {
            printf("Second Handshake send error\n");
            disconnect(sock);
            return;
        }

        for (int i = 0; i < 8; i++)
        {
            sockaddr_in client_UDP_address = {};
            client_UDP_address.sin_family = AF_INET;
            client_UDP_address.sin_port = htons(ports_buffer[i]);
            client_UDP_address.sin_addr = get_peer_address(sock).sin_addr;
            client_UDP_addresses.push_back(client_UDP_address);
        }

        // end of second handshake

        // third handshake, confirm the stream id with client

        // end of third handshake

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
    MTCS_client_controller_socket(string server_ip, int server_port) : cpp_tcp_socket_client(server_ip, server_port) {}
};

// this is the UDP socket, try to use this to transport data, it is faster, conenction less, and cost less resources
class MTCS_transportation_socket : public cpp_udp_socket
{
public:
    MTCS_transportation_socket(int port = 0) : cpp_udp_socket(port) {}
};