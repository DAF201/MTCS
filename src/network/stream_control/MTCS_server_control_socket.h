#ifndef MTCS_SERVER_CONTROL_SOCKET_HPP
#define MTCS_SERVER_CONTROL_SOCKET_HPP
#include <unordered_set>
#include "../sockets/tcp_server_socket_safe.h"
#include "MTCS_transport_socket.h"
const static string handshake_data_1 = "MTCSHS1";
const static string handshake_okay_1 = "HSOKAY1";
const static string handshake_okay_2 = "HSOKAY2";
const static string handshake_okay_3 = "HSOKAY3";
const static DWORD handshake_timeout_set = 5000;
const static DWORD handshake_timeout_clear = 0;

class MTCS_server_control_socket : public cpp_tcp_socket_server
{
private:
    static unordered_set<uint16_t> stream_id_set;

    bool handshake_1(SOCKET sock)
    {
        // set 5 second handshake to comfirm the reliability of the connection
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&handshake_timeout_set, sizeof(handshake_timeout_set));
        // recv the client first handshake
        char buffer[8];
        recv(sock, buffer, 8, 0);
        if (string(buffer) != handshake_data_1)
            return false;
        // send the server first handshake
        send(sock, handshake_data_1.c_str(), 8, 0);
        // recv client first comfirm
        recv(sock, buffer, 8, 0);
        if (string(buffer) != handshake_okay_1)
            return false;
        // send client first comfirm
        send(sock, handshake_okay_1.c_str(), 8, 0);
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&handshake_timeout_clear, sizeof(handshake_timeout_clear));
        printf("Handshake 1 success\n");
        return true;
    }

    bool handshake_2(SOCKET sock)
    {
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&handshake_timeout_set, sizeof(handshake_timeout_set));
        uint16_t stream_id_buffer;
        uint16_t stread_id_check;

        char buffer[8];
        while (true)
        {
            stream_id_buffer = rand() % 65535;
            if (stream_id_set.count(stream_id_buffer) == 0)
            {
                stream_id_set.insert(stream_id_buffer);
                break;
            }
        }
        printf("%d\n", stream_id_buffer);
        send(sock, (char *)&stream_id_buffer, 16, 0);
        recv(sock, (char *)&stread_id_check, 16, 0);
        printf("%d\n", stread_id_check);
        if (stream_id_buffer != stread_id_check)
            return false;

        send(sock, handshake_okay_2.c_str(), handshake_okay_2.length(), 0);

        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&handshake_timeout_clear, sizeof(handshake_timeout_clear));
        return true;
    }

    bool handshake_3()
    {
        return true;
    }

    bool handshake(SOCKET sock)
    {
        return handshake_1(sock) && handshake_2(sock) && handshake_3();
    }

public:
    MTCS_server_control_socket(int port) : cpp_tcp_socket_server(port)
    {
    }

    bool pre_connection_handler(SOCKET sock) override
    {
        if (handshake(sock))
        {
            return true;
        }
        return false;
    }

    void connection_handler(SOCKET sock, char *buffer, int size) {
    };
};
unordered_set<uint16_t> MTCS_server_control_socket::stream_id_set;
#endif