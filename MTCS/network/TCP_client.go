package network

import (
	"fmt"
	"net"
)

type TCPClient struct {
	RemoteAddress  string
	RemotePort     uint16
	LocalPort      uint16
	ReadBuffer     []byte
	LocalUDPPorts  []uint16
	ServerUDPPorts []uint16
}

func (socket *TCPClient) Connect(remoteAddress string, remotePort int) {
	socket.RemoteAddress = remoteAddress
	socket.RemotePort = uint16(remotePort)
}

func (socket *TCPClient) Init(localPort int, remoteAddress string, remotePort int) {
	socket.LocalPort = uint16(localPort)
}
