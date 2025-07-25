package network

import (
	"fmt"
	"log"
	"net"
)

type UDPServer struct {
	LocalPort  uint16
	Socket     net.UDPConn
	ReadBuffer []byte
}

func (socket *UDPServer) Listen() bool {
	localAddr := net.UDPAddr{
		IP:   net.IPv4zero,
		Port: int(socket.LocalPort),
	}
	Conn, err := net.ListenUDP("udp", &localAddr)

	if err != nil {
		log.Println("UDP Server Listen error")
		return false
	}
	socket.Socket = *Conn
	return true
}

func (socket *UDPServer) Init(LocalPort uint16) {
	socket.LocalPort = LocalPort
	socket.Listen()

}
