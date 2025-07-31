#include "../network/stream_control/MTCS_stream_control.h"
using namespace std;

int main()
{
    // auto s = MTCS_server_controller_socket(1024);
    // s.start();
    char test_data[] = "HELLO";
    MTCS_transportation_socket s = MTCS_transportation_socket(1011);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1010);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    printf(s.recv_packet().data.get());
    s.send_packet(test_data, 6, addr);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
