#include "../network/stream_control/MTCS_stream_control.h"
using namespace std;

int main()
{
    auto s = MTCS_server_controller_socket(1024);
    s.start();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
