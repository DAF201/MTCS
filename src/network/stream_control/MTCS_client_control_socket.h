#ifndef MTCS_CLIENT_CONTROL_SOCKET_HPP
#define MTCS_CLIENT_CONTROL_SOCKET_HPP
#include <unordered_set>
#include "../sockets/tcp_client_socket_safe.h"
#include "MTCS_transport_socket.h"
const static string handshake_data_1 = "MTCSHS1";
const static string handshake_okay_1 = "HSOKAY1";
const static string handshake_okay_2 = "HSOKAY2";
const static string handshake_okay_3 = "HSOKAY3";
const static DWORD handshake_timeout_set = 5000;
const static DWORD handshake_timeout_clear = 0;

class MTCS_client_control_socket : public cpp_tcp_socket_client
{
private:
    uint16_t this_stream_id;

    bool handshake_1()
    {
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&handshake_timeout_set, sizeof(handshake_timeout_set));
        // send the client the first handshake
        char buffer[8];
        send(sock, handshake_data_1.c_str(), 8, 0);
        // recv the server first handshake
        recv(sock, buffer, 8, 0);
        if (string(buffer) != handshake_data_1)
            return false;
        // send server first comfirm
        send(sock, handshake_okay_1.c_str(), 8, 0);
        // recv server first comfirm
        recv(sock, buffer, 8, 0);
        if (string(buffer) != handshake_okay_1)
            return false;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&handshake_timeout_clear, sizeof(handshake_timeout_clear));
        printf("Handshake 1 success\n");
        return true;
    }
    bool handshake_2()
    {
        string buffer = handshake_okay_1;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&handshake_timeout_set, sizeof(handshake_timeout_set));
        recv(sock, (char *)&this_stream_id, 16, 0);
        printf("%d\n", this_stream_id);
        send(sock, (char *)&this_stream_id, 16, 0);

        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&handshake_timeout_clear, sizeof(handshake_timeout_clear));
        return true;
    }
    bool handshake_3()
    {
        return true;
    }

    bool handshake()
    {
        return handshake_1() && handshake_2() && handshake_3();
    }

public:
    MTCS_client_control_socket(string address, int port) : cpp_tcp_socket_client(address, port)
    {
        if (handshake())
        {
            printf("Handshake success\n");
        };
    }
};
#endif