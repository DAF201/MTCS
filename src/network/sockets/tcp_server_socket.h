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
#include <algorithm>
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

    mutex connections_list_mutex;
    vector<SOCKET> clients_connections_list;
    queue<socket_pkg> send_queue;
    queue<socket_pkg> recv_queue;

    mutex send_mutex;
    condition_variable send_cv;

    mutex recv_mutex;
    condition_variable recv_cv;

    atomic<bool> stop_flag;

    virtual void connection_handler_func(SOCKET sock) = 0;

    void connection_handler(SOCKET sock)
    {
        // TODO: do something
        connection_handler_func(sock);
        // complete, disconnected
        auto client_socket = std::find(clients_connections_list.begin(), clients_connections_list.end(), sock);
        if (client_socket != clients_connections_list.end())
            clients_connections_list.erase(client_socket);
        closesocket(sock);
        ReleaseSemaphore(connection_sem, 1, NULL);
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
        ACCEPT_THREAD = thread(&cpp_socket_server::accept_loop, this);
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
                ReleaseSemaphore(connection_sem, 1, NULL);
                continue;
            }
            lock_guard<mutex> lock(connections_list_mutex);
            clients_connections_list.push_back(sock);
            thread(&cpp_socket_server::connection_handler, this, sock).detach();
        }
    }
    ~cpp_socket_server()
    {
        quit();
        socket_wsa_end();
    }

    void quit()
    {
        stop_flag = true;
        system("powershell -Command \"& { $client = New-Object System.Net.Sockets.TcpClient('127.0.0.1', 8000); $client.Close() }\"");
    }
};