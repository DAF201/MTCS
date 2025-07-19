#ifndef CONTROL_SOCKET
#define CONTROL_SOCKET
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <time.h>
#include "../protocol/MTCS.h"
#pragma comment(lib, "ws2_32.lib")
extern const uint16_t packet_data_max_length;
extern const uint8_t packet_header_length;
const char exit_command[8] = {'M', 'T', 'C', 'S', 'E', 'X', 'I', 'T'};

class control_socket_s
{
    control_socket_s(int local_port, uint16_t stream_id, uint8_t channel_id)
    {
        server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP);
        if (server_socket == INVALID_SOCKET)
        {
            throw std::runtime_error("Socket creation failed");
        }
        sockaddr_in server_address = {};
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(local_port);
        server_address.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_socket, (sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR)
        {
            closesocket(server_socket);
            throw std::runtime_error("Socket binding failed");
        }

        // TODO complete the control TCP socket

        this->stream_data_buffer = stream_data_buffer;
        this->stream_id = stream_id;
        this->channel_id = channel_id;
        this->local_port = local_port;
    }

    void send_command()
    {
    }

    void recv_command()
    {
    }

private:
    char buffer[packet_header_length + packet_data_max_length];
    sockaddr_in client_address;
    socklen_t client_address_length = sizeof(client_address);
    int recv_length;
    SOCKET server_socket;
    char *stream_data_buffer;
    uint8_t channel_id;
    uint16_t stream_id;
    int local_port;
};

class control_socket_c
{
};
#endif