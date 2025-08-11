// #include "../network/stream_control/MTCS_stream_control.h"
#include "../network/stream_control/MTCS_server_control_socket.h"
using namespace std;

int main()
{
    char test_data[] = "HELLO";

    MTCS_server_control_socket s = MTCS_server_control_socket(1024);
    s.start();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
