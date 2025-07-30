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

// semaphore init
static HANDLE connection_sem = CreateSemaphore(NULL, MAX_CONNECTIONS_COUNT, MAX_CONNECTIONS_COUNT, NULL);

class cpp_socket_server
{

protected:
    // packet data
    struct socket_pkg
    {
        char *data = nullptr;
        int size = 0;
        SOCKET client_socket;
    };

    SOCKET server_listener_socket = INVALID_SOCKET;
    WORD sock_version;
    WSADATA WSA_data;
    int server_port = 0;

    thread ACCEPT_THREAD;
    thread SEND_THREAD;

    mutex connections_list_mutex;
    vector<SOCKET> clients_connections_list;
    queue<socket_pkg> send_queue;
    queue<socket_pkg> recv_queue;

    mutex send_mutex;
    condition_variable send_cv;

    mutex recv_mutex;
    condition_variable recv_cv;

    atomic<bool> stop_flag;

    // handler need to be implemented by child class
    virtual void connection_handler(SOCKET sock) = 0;

    void _connection_handler(SOCKET sock)
    {
        // TODO: do something
        connection_handler(sock);
        // complete,remove socket from connection list, disconnected
        lock_guard<mutex>
            lock(connections_list_mutex);
        auto client_socket = std::find(clients_connections_list.begin(), clients_connections_list.end(), sock);
        if (client_socket != clients_connections_list.end())
            clients_connections_list.erase(client_socket);
        closesocket(sock);
        // release semaphore
        ReleaseSemaphore(connection_sem, 1, NULL);
    }

    void send_loop()
    {
        // sending packet loop
        while (!stop_flag)
        {
            // wait for signal of new packet in queue, then get the front of the queue
            unique_lock<mutex> lock(send_mutex);
            send_cv.wait(lock, [this]
                         { return !send_queue.empty() || stop_flag; });
            if (stop_flag)
                break;
            auto pkg = send_queue.front();
            send_queue.pop();
            lock.unlock();
            // send data
            int sent = 0;
            while (sent < pkg.size)
            {
                int ret = send(pkg.client_socket, pkg.data + sent, pkg.size - sent, 0);
                if (ret == SOCKET_ERROR)
                {
                    printf("Send error: %d\n", WSAGetLastError());
                    break;
                }
                sent += ret;
            }
            delete[] pkg.data;
        }
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

        // bind listener
        if (bind(server_listener_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            printf("Bind failed: %d\n", WSAGetLastError());
            closesocket(server_listener_socket);
            socket_wsa_end();
            throw runtime_error("Bind failed");
        }

        // listen
        if (listen(server_listener_socket, SOMAXCONN) == SOCKET_ERROR)
        {
            printf("Listen failed: %d\n", WSAGetLastError());
            closesocket(server_listener_socket);
            socket_wsa_end();
            throw runtime_error("Listen failed");
        }

        printf("Server listening on port %d\n", server_port);
        // accept connections
        ACCEPT_THREAD = thread(&cpp_socket_server::accept_loop, this);

        // sending data
        SEND_THREAD = thread(&cpp_socket_server::send_loop, this);
    }

    void accept_loop()
    {
        while (!stop_flag)
        {
            // wait for semaphore to ensure the connections at a time is no more then 8
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
            thread(&cpp_socket_server::_connection_handler, this, sock).detach();
        }
    }

    void send_packet(char *data, int size, SOCKET client_socket)
    {
        char *buffer = new char[size];
        memcpy(buffer, data, size);
        socket_pkg pkg = {buffer, size, client_socket};
        unique_lock<mutex> lock(send_mutex);
        send_queue.push(pkg);
        lock.unlock();
        send_cv.notify_one();
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
        ACCEPT_THREAD.join();
        SEND_THREAD.join();
    }
};