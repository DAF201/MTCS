#ifndef SOCKET_SETUP_H
#define SOCKET_SETUP_H

#include <winsock2.h>
#include <stdexcept>
void socket_wsa_start()
{
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        throw std::runtime_error("WSAStartup failed");
}

void socket_wsa_end()
{
    WSACleanup();
}

#endif