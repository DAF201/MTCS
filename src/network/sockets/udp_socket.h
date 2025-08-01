#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <queue>
#include <atomic>
#include "socket_setup.h"

using namespace std;
class cpp_udp_socket
{

protected:
    // packet data, store the client information, data, and size of data
    struct socket_pkg
    {
        unique_ptr<char[]> data;
        int size = 0;
        sockaddr_in target_addr;
    };

    // socket info
    SOCKET local_socket = INVALID_SOCKET;
    WORD sock_version;
    WSADATA WSA_data;
    int port = 0;

    atomic<bool> stop_flag;

public:
    cpp_udp_socket(int port = 0) : stop_flag(false)
    {
        sock_version = MAKEWORD(2, 2);

        socket_wsa_start();

        local_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (local_socket == INVALID_SOCKET)
        {
            printf("Error creating socket\n");
            socket_wsa_end();
            throw runtime_error("Socket creation failed");
        }

        sockaddr_in server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        // bind to an available port
        if (bind(local_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            printf("Bind failed: %d\n", WSAGetLastError());
            closesocket(local_socket);
            socket_wsa_end();
            throw runtime_error("Bind failed");
        }

        // get which port being assigned by os
        int server_addr_size = sizeof(server_addr);
        if (getsockname(local_socket, (sockaddr *)&server_addr, &server_addr_size) == SOCKET_ERROR)
        {
            printf("Failed to get socket info\n");
            closesocket(local_socket);
            socket_wsa_end();
            throw runtime_error("Fetch data failed");
        }
        this->port = ntohs(server_addr.sin_port);
    }

    // this is a point to point UDP, so just let the child class handle the packet logic
    bool send_packet(const char *data, int size, const sockaddr_in &target_address)
    {
        if (stop_flag)
        {
            return false;
        }

        int ret = sendto(local_socket, data, size, 0,
                         reinterpret_cast<const sockaddr *>(&target_address),
                         sizeof(target_address));
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
        sockaddr_in target_address{};
        int target_address_length = sizeof(target_address);
        char buffer[MAX_PACKET_LENGTH];
        int ret = recvfrom(local_socket, buffer, MAX_PACKET_LENGTH, 0,
                           (sockaddr *)&target_address, &target_address_length);
        if (ret == SOCKET_ERROR)
        {
            printf("recvfrom failed: %d\n", WSAGetLastError());
            return socket_pkg();
        }
        socket_pkg packet;
        packet.size = ret;
        packet.data = make_unique<char[]>(ret);
        memcpy(packet.data.get(), buffer, ret);
        packet.target_addr = target_address;
        return packet;
    }

    int get_port()
    {
        return port;
    }

    void quit()
    {
        string cmd = "powershell -Command \"$udpClient = New-Object System.Net.Sockets.UdpClient; $udpClient.Connect('127.0.0.1'," + to_string(port) + "); $bytes = [System.Text.Encoding]::ASCII.GetBytes('exit'); $udpClient.Send($bytes, $bytes.Length); $udpClient.Close()\"";
        system(cmd.c_str());
    }

    ~cpp_udp_socket() noexcept
    {
        stop_flag = true;
        if (local_socket != INVALID_SOCKET)
        {
            closesocket(local_socket);
            local_socket = INVALID_SOCKET;
        }
        socket_wsa_end();
    }
};