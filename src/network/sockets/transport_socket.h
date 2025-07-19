#ifndef TRANSPORT_SOCKET
#define TRANSPORT_SOCKET
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <time.h>
#include "../protocol/MTCS.h"
extern const uint16_t packet_data_max_length;
extern const uint8_t packet_header_length;
const char exit_command[8] = {'M', 'T', 'C', 'S', 'E', 'X', 'I', 'T'};
class transport_socket_s
{
public:
    transport_socket_s(int local_port, uint16_t stream_id, uint8_t channel_id, char *stream_data_buffer)
    {

        server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
        this->stream_data_buffer = stream_data_buffer;
        this->stream_id = stream_id;
        this->channel_id = channel_id;
        this->local_port = local_port;
    }
    // recv_map need at least (max_packets + 7) / 8 bytes space
    void start_recv(char *recv_map, uint32_t max_packets)
    {
        while (true)
        {
            recv_length = recvfrom(server_socket, buffer, sizeof(buffer), 0, (sockaddr *)&client_address, &client_address_length);
            if (recv_length == SOCKET_ERROR)
            {
                continue;
            }
            char addr_buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_address.sin_addr), addr_buf, INET_ADDRSTRLEN);
            if (recv_length >= 8 &&
                std::string(addr_buf) == "127.0.0.1" &&
                memcmp(exit_command, buffer, 8) == 0)
            {
                break;
            }
            else
            {
                std::vector<char> MTCS_vector = MTCS_copy(buffer, recv_length);
                MTCS header = MTCS_header_extract(MTCS_vector);
                if (recv_length != header.packet_data_length + packet_header_length)
                    continue;
                if (header.target_port != local_port)
                    continue;
                if ((header.stream_id != stream_id) || (header.channel_id != channel_id))
                    continue;

                MTCS_parse(MTCS_vector, stream_data_buffer);

                if (header.packet_index < max_packets)
                {
                    recv_map[header.packet_index / 8] |= (1 << (header.packet_index % 8));
                }
            }
        }
    }

    ~transport_socket_s()
    {
        closesocket(server_socket);
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

class transport_socket_c
{
public:
    transport_socket_c(std::string host, int target_port, int binding_port, uint16_t stream_id, uint8_t channel_id)
    {

        client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (client_socket == INVALID_SOCKET)
        {
            throw std::runtime_error("Socket creation failed");
        }

        sockaddr_in localAddr{};
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(binding_port);
        localAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(client_socket, (sockaddr *)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
        {
            std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
            closesocket(client_socket);
            throw std::runtime_error("Socket binding failed");
        }

        server_address = {};
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(target_port);
        inet_pton(AF_INET, host.c_str(), &server_address.sin_addr);

        this->stream_id = stream_id;
        this->channel_id = channel_id;
        this->local_port = binding_port;
        this->target_port = target_port;
    }

    void send_data(char *data, uint16_t data_length, uint8_t resend_attempt,
                   uint32_t stream_packet_count, uint16_t packet_index)
    {
        std::vector<char> MTCS_packet = MTCS_serialize(
            local_port, target_port,
            stream_id, channel_id,
            resend_attempt, stream_packet_count,
            data_length, packet_index,
            time(NULL), data);

        sendto(client_socket,
               MTCS_packet.data(),
               MTCS_packet.size(),
               0,
               (sockaddr *)&server_address,
               sizeof(server_address));
        Sleep(1);
    }

    ~transport_socket_c()
    {
        closesocket(client_socket);
    }

private:
    SOCKET client_socket;
    sockaddr_in server_address;
    int local_port;
    int target_port;
    uint8_t channel_id;
    uint16_t stream_id;
};
#endif