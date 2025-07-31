#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <queue>
#include <atomic>
#include "socket_setup.h"

using namespace std;
class cpp_udp_socket_server
{

protected:
    // packet data, store the client information, data, and size of data
    struct socket_pkg
    {
        unique_ptr<char[]> data;
        int size = 0;
        sockaddr_in client_addr;
    };

    // socket info
    SOCKET server_socket = INVALID_SOCKET;
    WORD sock_version;
    WSADATA WSA_data;
    int server_port = 0;

    atomic<bool> stop_flag;

public:
    cpp_udp_socket_server() : stop_flag(false)
    {
        sock_version = MAKEWORD(2, 2);

        socket_wsa_start();

        server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (server_socket == INVALID_SOCKET)
        {
            printf("Error creating socket\n");
            socket_wsa_end();
            throw runtime_error("Socket creation failed");
        }

        sockaddr_in server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(0);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        // bind to an available port
        if (bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            printf("Bind failed: %d\n", WSAGetLastError());
            closesocket(server_socket);
            socket_wsa_end();
            throw runtime_error("Bind failed");
        }

        // get which port being assigned by os
        int server_addr_size = sizeof(server_addr);
        if (getsockname(server_socket, (sockaddr *)&server_addr, &server_addr_size) == SOCKET_ERROR)
        {
            printf("Failed to get socket info\n");
            closesocket(server_socket);
            socket_wsa_end();
            throw runtime_error("Fetch data failed");
        }
        this->server_port = ntohs(server_addr.sin_port);
    }

    // this is a point to point UDP, so just let the child class handle the packet logic
    bool send_packet(const char *data, int size, const sockaddr_in &client_address)
    {
        if (stop_flag)
        {
            return false;
        }

        int ret = sendto(server_socket, data, size, 0,
                         reinterpret_cast<const sockaddr *>(&client_address),
                         sizeof(client_address));
        if (ret == SOCKET_ERROR)
        {
            printf("sendto failed: %d\n", WSAGetLastError());
            return false;
        }
        if (ret != size)
        {
            printf("partial send? sent %d of %d\n", ret, size);
            return false;
        }
        return true;
    }

    // recv one packet, since UDP is packet based, and will not mix with the next packet
    socket_pkg recv_packet()
    {
        if (stop_flag)
        {
            return socket_pkg();
        }
        sockaddr_in client_address{};
        int client_address_length = sizeof(client_address);
        char buffer[MAX_PACKET_LENGTH];
        int ret = recvfrom(server_socket, buffer, MAX_PACKET_LENGTH, 0,
                           (sockaddr *)&client_address, &client_address_length);
        if (ret == SOCKET_ERROR)
        {
            printf("recvfrom failed: %d\n", WSAGetLastError());
            return socket_pkg();
        }
        socket_pkg packet;
        packet.size = ret;
        packet.data = make_unique<char[]>(ret);
        memcpy(packet.data.get(), buffer, ret);
        packet.client_addr = client_address;
        return packet;
    }

    void quit()
    {
        string cmd = "powershell -Command \"$udpClient = New-Object System.Net.Sockets.UdpClient; $udpClient.Connect('127.0.0.1'," + to_string(server_port) + "); $bytes = [System.Text.Encoding]::ASCII.GetBytes('exit'); $udpClient.Send($bytes, $bytes.Length); $udpClient.Close()\"";
        system(cmd.c_str());
    }

    ~cpp_udp_socket_server()
    {
        stop_flag = true;
        if (server_socket != INVALID_SOCKET)
        {
            shutdown(server_socket, SD_BOTH);
            closesocket(server_socket);
            server_socket = INVALID_SOCKET;
        }
        socket_wsa_end();
    }
};