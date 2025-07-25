#ifndef SOCKET_SETUP_H
#define SOCKET_SETUP_H

#include <winsock2.h>
#include <stdexcept>
#include <mutex>

// make sure the WSA only create and destroy once per process
inline void socket_wsa_start()
{
    static bool started = false;
    static std::mutex mtx;

    std::lock_guard<std::mutex> lock(mtx);
    if (started)
        return;

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        throw std::runtime_error("WSAStartup failed");

    started = true;
}

inline void socket_wsa_end()
{
    static bool started = true;
    static std::mutex mtx;

    std::lock_guard<std::mutex> lock(mtx);
    if (!started)
        return;

    WSACleanup();
    started = false;
}

#endif
