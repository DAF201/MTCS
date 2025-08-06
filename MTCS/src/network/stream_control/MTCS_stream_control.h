#ifndef MTCS_STREAM_CONTROL_HPP
#define MTCS_STREAM_CONTROL_HPP
#include "../protocol/MTCS.h"
#include "../sockets/tcp_server_socket_safe.h"
#include "../sockets/tcp_client_socket_safe.h"
#include "../sockets/udp_socket.h"
#include <random>
#include <unordered_set>
#define MAX_PACKET_LENGTH 1056
using namespace std;
const char MTCS_SERVER_HANDSHAKE_1[8] = "MTCSSH1";
const char MTCS_CLIENT_HANDSHAKE_1[8] = "MTCSCH1";

// this is the UDP socket, try to use this to transport data, it is faster, conenction less, and cost less resources
class MTCS_transportation_socket : public cpp_udp_socket
{

public:
    MTCS_transportation_socket(int port = 0) : cpp_udp_socket(port)
    {
    }
};

// this is the TCP controller socket, try to limite the size of data send, use the UDP sockets to transport data
class MTCS_server_controller_socket : public cpp_tcp_socket_server
{

private:
    static unordered_set<uint16_t> stream_id_set;
    uint16_t this_stream_id;
    // save the UDP sockets of server and client address to send to
    vector<unique_ptr<MTCS_transportation_socket>> server_UDP_sockets;
    vector<sockaddr_in> client_UDP_addresses;

public:
    MTCS_server_controller_socket(int port) : cpp_tcp_socket_server(port) {}

    bool pre_connection_handler(SOCKET sock)
    {
        // 3 seconds timeout
        DWORD timeout_ms = 3000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));

        // first handshake, confirm the client can send and understand the handshake

        unique_ptr<char[]> buffer = make_unique<char[]>(MAX_PACKET_LENGTH);
        int ret = recv(sock, buffer.get(), 8, 0);

        if (ret == SOCKET_ERROR)
        {
            printf("First handshake socket recv error\n");
            return false;
        }

        if (ret < 8 || memcmp(buffer.get(), MTCS_CLIENT_HANDSHAKE_1, 8) != 0)
        {
            // disconnect if the handshake failed
            printf("First handshake data not match\n");
            return false;
        }

        memcpy(buffer.get(), MTCS_SERVER_HANDSHAKE_1, 8);

        ret = send(sock, buffer.get(), 8, 0);


        if (ret == SOCKET_ERROR)
        {
            printf("First handshake socket send error\n");
            return false;
        }

        // end of the first handshake

        // second handshake, check the ports with clients (create 8 udp ports here)

        // create UDP sockets, and get ports to client
        int ports_buffer[8];
        for (int i = 0; i < 8; i++)
        {
            auto udp_socket = make_unique<MTCS_transportation_socket>();
            ports_buffer[i] = udp_socket->get_port();
            server_UDP_sockets.push_back(std::move(udp_socket));
        }

        // send the ports to client, then wait for client to reply for the ports going to use
        ret = send(sock, (char *)ports_buffer, sizeof(ports_buffer), 0);
        if (ret == SOCKET_ERROR)
        {
            printf("Second handshake socket send error\n");
            return false;
        }

        // recv the client's response about which ports it is going to use
        ret = recv(sock, (char *)ports_buffer, sizeof(ports_buffer), 0);

        if (ret == SOCKET_ERROR)
        {
            printf("Second handshake socket recv error\n");
            return false;
        }

        if (ret != sizeof(ports_buffer))
        {
            printf("Second handshake data not matching expected length\n");
            return false;
        }

        // read the client sockets, create address for them
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

        // generate a new stream id not being used
        uint16_t new_stream_id = rand() % 65535;
        while (stream_id_set.count(new_stream_id))
        {
            new_stream_id = rand() % 65535;
        }
        stream_id_set.insert(new_stream_id);
        this_stream_id = new_stream_id;

        // send client the stream id
        ret = send(sock, (char *)&this_stream_id, MAX_PACKET_LENGTH, 0);
        if (ret == SOCKET_ERROR)
        {
            printf("Third handshake socket send error\n");
            return false;
        }

        uint16_t stream_id_buffer;
        ret = recv(sock, (char *)&stream_id_buffer, sizeof(stream_id_buffer), 0);
        if (ret == SOCKET_ERROR)
        {
            printf("Third handshake socket recv error\n");
            return false;
        }

        if (stream_id_buffer != this_stream_id)
        {
            printf("Third handshake stream id not match\n");
            return false;
        }
        // end of third handshake

        // clear the timeout after handshake
        timeout_ms = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));
        return true;
    }

    void start()
    {
        cpp_tcp_socket_server::start();
    }

    ~MTCS_server_controller_socket()
    {
        // remove the stream id at closing
        stream_id_set.erase(this_stream_id);
    }
};

unordered_set<uint16_t> MTCS_server_controller_socket::stream_id_set;

class MTCS_client_controller_socket : public cpp_tcp_socket_client
{
private:
    uint16_t this_stream_id;
    // save the UDP sockets of server and client address to send to
    std::vector<std::unique_ptr<MTCS_transportation_socket>> server_UDP_sockets;
    vector<sockaddr_in> server_UDP_addresses;
    bool handshake()
    {
        // 3 seconds timeout
        DWORD timeout_ms = 3000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));

        // first handshake, confirm the server can send and can understant the handshake
        unique_ptr<char[]> buffer = make_unique<char[]>(MAX_PACKET_LENGTH);
        memcpy(buffer.get(), MTCS_CLIENT_HANDSHAKE_1, 8);

        // send first handshake to server
        int ret = send(sock, buffer.get(), 8, 0);
        if (ret == SOCKET_ERROR)
        {
            printf("First handshake socket send error\n");
            return false;
        }

        ret = recv(sock, buffer.get(), 8, 0);

        if (ret == SOCKET_ERROR)
        {
            printf("First handshake recv error\n");
            return false;
        }

        if (ret < 8 || memcmp(buffer.get(), MTCS_SERVER_HANDSHAKE_1, 8) != 0)
        {
            printf("First handshake data not match\n");
            return false;
        }
        // end of the first handshake

        // second handshake, check the ports with server (create 8 udp ports here)
        // create UDP sockets, and get ports to server
        int ports_buffer[8];

        // recv the ports server is going to use the in buffer, when reply server with ports going to use
        ret = recv(sock, (char *)ports_buffer, sizeof(ports_buffer), 0);

        if (ret == SOCKET_ERROR)
        {
            printf("Second handshake socket recv error\n");
            return false;
        }
        if (ret != sizeof(ports_buffer))
        {
            printf("Second handshake data length not matching\n");
            return false;
        }

        // read the server sockets, create address for them
        for (int i = 0; i < 8; i++)
        {
            sockaddr_in server_UDP_addresse = {};
            server_UDP_addresse.sin_family = AF_INET;
            server_UDP_addresse.sin_port = htons(ports_buffer[i]);
            server_UDP_addresse.sin_addr = get_peer_address(sock).sin_addr;
            server_UDP_addresses.push_back(server_UDP_addresse);
        }

        // create 8 UDP sockets, get the ports
        for (int i = 0; i < 8; i++)
        {
            auto udp_socket = make_unique<MTCS_transportation_socket>();
            ports_buffer[i] = udp_socket->get_port();
            server_UDP_sockets.push_back(std::move(udp_socket));
        }
        ret = send(sock, (char *)ports_buffer, sizeof(ports_buffer), 0);
        if (ret == SOCKET_ERROR)
        {
            printf("Second handshake socket send error\n");
            return false;
        }

        // end of the second handshake

        // third handshake, confirm the stream id
        ret = recv(sock, (char *)&this_stream_id, MAX_PACKET_LENGTH, 0);
        if (ret == SOCKET_ERROR)
        {
            printf("Third handshake socket recv error\n");
            return false;
        }
        if (ret != sizeof(this_stream_id))
        {
            printf("Third handshake data length not match\n");
            return false;
        }
        ret = send(sock, (char *)&this_stream_id, sizeof(this_stream_id), 0);
        if (ret == SOCKET_ERROR)
        {
            printf("Third handshake socket send error\n");
            return false;
        }
        // end of the third handshake
        return true;
    }

public:
    MTCS_client_controller_socket(string server_ip, int server_port) : cpp_tcp_socket_client(server_ip, server_port)
    {
        if (handshake() == false)
        {
            stop_flag = true;
            quit();
            closesocket(sock);
            return;
        }
        else
        {
            printf("client handshake successed\n");
        }
    }
};
#endif