#ifndef MTCS_TRANSPORT_SOCKET_HPP
#define MTCS_TRANSPORT_SOCKET_HPP
#include "../sockets/udp_socket.h"
class MTCS_transport_socket : public cpp_udp_socket
{

public:
    MTCS_transport_socket(int port = 0) : cpp_udp_socket(port)
    {
    }
};

#endif