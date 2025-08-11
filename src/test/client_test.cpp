// #include "../network/stream_control/MTCS_stream_control.h"
#include "../network/stream_control/MTCS_client_control_socket.h"
using namespace std;

int main()
{
    char test_data[] = "HELLO";

    MTCS_client_control_socket s = MTCS_client_control_socket("127.0.0.1", 1024);

    // while (true)
    // {
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }
}
