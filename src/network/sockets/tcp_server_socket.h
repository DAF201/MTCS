#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <semaphore>
#include "socket_setup.h"
#define MAX_CONNECTIONS_COUNT 8
#pragma comment(lib, "ws2_32.lib")

using namespace std;

static HANDLE connection_sem = CreateSemaphore(NULL, MAX_CONNECTIONS_COUNT, MAX_CONNECTIONS_COUNT, NULL);

class cpp_socket_server
{

protected:
    struct socket_pkg
    {
        char *data = nullptr;
        int size = 0;
        SOCKET client_id;
    };

    SOCKET server_listener_socket = INVALID_SOCKET;
    WORD sock_version;
    WSADATA WSA_data;
    int server_port = 0;

    thread ACCEPT_THREAD;

    vector<SOCKET> clients_connections_list;
    queue<socket_pkg> send_queue;
    queue<socket_pkg> recv_queue;

    mutex send_mutex;
    condition_variable send_cv;

    mutex recv_mutex;
    condition_variable recv_cv;

    atomic<bool> stop_flag;

    void connection_handler()
    {
    }

public:
    cpp_socket_server(const int server_port) : stop_flag(false)
    {
        sock_version = MAKEWORD(2, 2);

        socket_wsa_start();

        server_listener_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_listener_socket == INVALID_SOCKET)
        {
            printf("Error creating socket\n");
            socket_wsa_end();
            throw runtime_error("Socket creation failed");
        }

        this->server_port = server_port;

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_listener_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            printf("Bind failed: %d\n", WSAGetLastError());
            closesocket(server_listener_socket);
            socket_wsa_end();
            throw runtime_error("Bind failed");
        }

        if (listen(server_listener_socket, SOMAXCONN) == SOCKET_ERROR)
        {
            printf("Listen failed: %d\n", WSAGetLastError());
            closesocket(server_listener_socket);
            socket_wsa_end();
            throw runtime_error("Listen failed");
        }

        printf("Server listening on port %d\n", server_port);
    }

    void accept_loop()
    {
        while (!stop_flag)
        {
            WaitForSingleObject(connection_sem, INFINITE);
            SOCKET sock = accept(server_listener_socket, nullptr, nullptr);
            if (sock == INVALID_SOCKET)
            {
                printf("Error creating socket\n");
                continue;
            }
            clients_connections_list.push_back(sock);
        }
    }
};