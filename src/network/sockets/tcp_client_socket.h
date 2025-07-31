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
#include "socket_setup.h"
#pragma comment(lib, "ws2_32.lib")

using namespace std;

class cpp_tcp_socket_client
{
public:
    // client packet, no need to save server socket since there is just one server
    struct socket_pkg
    {
        char *data = nullptr;
        int size = 0;
    };

protected:
    SOCKET client_socket = INVALID_SOCKET;
    WORD sock_version;
    WSADATA WSA_data;
    sockaddr_in server_address;

    thread RECV_THREAD;
    thread SEND_THREAD;

    queue<socket_pkg> send_queue;
    queue<socket_pkg> recv_queue;

    mutex send_mutex;
    condition_variable send_cv;

    mutex recv_mutex;
    condition_variable recv_cv;

    atomic<bool> stop_flag;

    void send_loop()
    {
        while (!stop_flag)
        {
            unique_lock<mutex> lock(send_mutex);
            send_cv.wait(lock, [this]
                         { return !send_queue.empty() || stop_flag; });
            if (stop_flag)
                break;

            auto pkg = send_queue.front();
            send_queue.pop();
            lock.unlock();

            int sent = 0;
            while (sent < pkg.size)
            {
                int ret = send(client_socket, pkg.data + sent, pkg.size - sent, 0);
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

    void recv_loop()
    {
        constexpr int BUFFER_SIZE = 1024;
        while (!stop_flag)
        {
            char buffer[BUFFER_SIZE];
            int received = recv(client_socket, buffer, BUFFER_SIZE, 0);

            if (received == 0)
            {
                printf("Connection closed by peer\n");
                stop_flag = true;
                break;
            }
            else if (received == SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK || err == WSAEINTR)
                {
                    this_thread::sleep_for(chrono::milliseconds(10));
                    continue;
                }
                printf("Recv error: %d\n", err);
                stop_flag = true;
                break;
            }
            else
            {
                char *data_copy = new char[received];
                memcpy(data_copy, buffer, received);
                {
                    lock_guard<mutex> lock(recv_mutex);
                    recv_queue.push(socket_pkg{data_copy, received});
                }
                recv_cv.notify_one();
            }
        }
    }

public:
    cpp_tcp_socket_client(const string &server_ip, int server_port)
        : stop_flag(false)
    {
        sock_version = MAKEWORD(2, 2);

        socket_wsa_start();

        client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (client_socket == INVALID_SOCKET)
        {
            printf("Error creating socket\n");
            socket_wsa_end();
            throw runtime_error("Socket creation failed");
        }

        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(server_port);

        if (inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr) != 1)
        {
            printf("Invalid IP address format\n");
            closesocket(client_socket);
            socket_wsa_end();
            throw runtime_error("Invalid IP");
        }

        if (connect(client_socket, (sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR)
        {
            printf("Connect error\n");
            closesocket(client_socket);
            socket_wsa_end();
            throw runtime_error("Connect failed");
        }

        RECV_THREAD = thread(&cpp_tcp_socket_client::recv_loop, this);
        SEND_THREAD = thread(&cpp_tcp_socket_client::send_loop, this);

        printf("Socket created and connected\n");
    }

    ~cpp_tcp_socket_client()
    {
        stop_flag = true;

        send_cv.notify_all();
        recv_cv.notify_all();

        if (SEND_THREAD.joinable())
            SEND_THREAD.join();

        if (RECV_THREAD.joinable())
            RECV_THREAD.join();

        if (client_socket != INVALID_SOCKET)
            closesocket(client_socket);

        socket_wsa_end();

        {
            lock_guard<mutex> lock(recv_mutex);
            while (!recv_queue.empty())
            {
                delete[] recv_queue.front().data;
                recv_queue.pop();
            }
        }

        {
            lock_guard<mutex> lock(send_mutex);
            while (!send_queue.empty())
            {
                delete[] send_queue.front().data;
                send_queue.pop();
            }
        }
    }

    void send_packet(const void *data, int size)
    {
        if (size <= 0 || data == nullptr)
            return;

        char *buffer = new char[size];
        memcpy(buffer, data, size);

        {
            lock_guard<mutex> lock(send_mutex);
            send_queue.push(socket_pkg{buffer, size});
        }
        send_cv.notify_one();
    }

    socket_pkg recv_packet()
    {
        lock_guard<mutex> lock(recv_mutex);
        if (recv_queue.empty())
            return socket_pkg{nullptr, 0};
        socket_pkg pkg = recv_queue.front();
        recv_queue.pop();
        return pkg;
    }

    int recv_buffer_size()
    {
        return this->recv_queue.size();
    }

    void free_recv(socket_pkg &pkg)
    {
        if (pkg.data)
        {
            delete[] pkg.data;
            pkg.data = nullptr;
            pkg.size = 0;
        }
    }

    void quit() noexcept
    {
        stop_flag = true;
        send_cv.notify_all();
        recv_cv.notify_all();
        std::scoped_lock lock(send_mutex, recv_mutex);
        while (!recv_queue.empty())
        {
            free_recv(recv_queue.front());
            recv_queue.pop();
        }
        while (!send_queue.empty())
        {
            free_recv(send_queue.front());
            send_queue.pop();
        }
        if (client_socket != INVALID_SOCKET)
        {
            closesocket(client_socket);
            client_socket = INVALID_SOCKET;
        }
    }
};
