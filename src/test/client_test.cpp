#include "../network/stream_control/MTCS_stream_control.h"
using namespace std;

int main()
{
    // MTCS_client_controller_socket s = MTCS_client_controller_socket("127.0.0.1", 1024);
    char test_data[] = "HELLO";
    // s.send_packet((void *)(test_data), 6);
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // MTCS_client_controller_socket::socket_pkg p = s.recv_packet();
    // printf("%s", p.data.get());

    MTCS_transportation_socket s = MTCS_transportation_socket(1010);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1011);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    s.send_packet(test_data, 6, addr);
    printf(s.recv_packet().data.get());
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
