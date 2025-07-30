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

// semaphore init, make sure at most 8 connections can be accepted at once
static HANDLE connection_sem = CreateSemaphore(NULL, MAX_CONNECTIONS_COUNT, MAX_CONNECTIONS_COUNT, NULL);

// server super class, may need to inherit later
class cpp_socket_server
{
protected:
    // packet data, store the client information, data, and size of data
    struct socket_pkg
    {
        char *data = nullptr;
        int size = 0;
        SOCKET client_socket;
    };

    // socket info
    SOCKET server_listener_socket = INVALID_SOCKET;
    WORD sock_version;
    WSADATA WSA_data;
    int server_port = 0;

    // one socket for accept new connection, and one socket for sending the packets
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
    virtual void connection_handler(SOCKET sock, char *buffer, int size) {
        // child class can override this part if needed
    };

    void _connection_handler(SOCKET sock)
    {
        char *buffer = new char[1056];
        // TODO: do something, must notify recv_cv
        while (true)
        {
            // recv at most 1056 bytes of data at once
            int size = recv(sock, buffer, 1056, 0);
            if (size == 0)
            {
                printf("Client disconnected\n");
                break;
            }

            if (size == SOCKET_ERROR)
            {
                printf("Recv error: %d\n", WSAGetLastError());
                break;
            }

            {
                // lock the queue
                lock_guard<mutex> lock(recv_mutex);
                // create socket_pkg
                char *pkg_data = new char[size];
                memcpy(pkg_data, buffer, size);
                socket_pkg pkg = {pkg_data, size, sock};
                // insert to queue
                recv_queue.push(pkg);
                // notify one thread waiting
                recv_cv.notify_one();
            }
            // if need to do something else
            connection_handler(sock, buffer, size);
        }

        delete[] buffer;
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

    void _send_loop()
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

    void _accept_loop()
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
    }

    virtual void start()
    {
        printf("Server listening on port %d\n", server_port);
        // accept connections
        ACCEPT_THREAD = thread(&cpp_socket_server::_accept_loop, this);

        // sending data, recving are detached in the accept thread (just recv from each client and put to queue)
        SEND_THREAD = thread(&cpp_socket_server::_send_loop, this);
    }

    virtual void send_packet(char *data, int size, SOCKET client_socket)
    {
        // push data to the packet queue, will be processed by another thread of sending
        char *buffer = new char[size];
        memcpy(buffer, data, size);
        socket_pkg pkg = {buffer, size, client_socket};
        unique_lock<mutex> lock(send_mutex);
        send_queue.push(pkg);
        lock.unlock();
        send_cv.notify_one();
    }

    virtual socket_pkg recv_packet()
    {
        // try to get one packet form the recv queue
        unique_lock<mutex> lock(recv_mutex);
        // wait until new data coming and notice, check if the queue is empty or server stopped
        recv_cv.wait(lock, [this]
                     { return !recv_queue.empty() || stop_flag; });
        if (stop_flag && recv_queue.empty())
            return socket_pkg();
        socket_pkg packet = recv_queue.front();
        recv_queue.pop();
        return packet;
    }

    virtual void quit()
    {
        lock_guard<mutex> lock(connections_list_mutex);
        for (auto &sock : clients_connections_list)
        {
            shutdown(sock, SD_BOTH);
            closesocket(sock);
        }
        clients_connections_list.clear();
        system("powershell -Command \"& { $client = New-Object System.Net.Sockets.TcpClient('127.0.0.1', 8000); $client.Close() }\"");
        ACCEPT_THREAD.join();
        SEND_THREAD.join();
    }
    
    virtual ~cpp_socket_server()
    {
        stop_flag = true;
        send_cv.notify_all();
        recv_cv.notify_all();
        quit();
        socket_wsa_end();
    }
};