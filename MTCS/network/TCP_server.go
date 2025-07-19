package network

import (
	"bytes"
	"fmt"
	"log"
	"net"
	"regexp"
	"strconv"
	"strings"
	"time"
)

var portPattern = regexp.MustCompile(`^(\d{1,5})(:\d{1,5})*$`)

const (
	ServerHello = "MTCSSH1"
	ClientHello = "MTCSCH1"
)

type TCPServer struct {
	LocalPort      uint16
	Socket         net.Conn
	Listener       net.Listener
	ReadBuffer     []byte
	LocalUDPPorts  []uint16
	ClientUDPPorts []uint16
}

func (socket *TCPServer) Listen() bool {
	listener, err := net.Listen("tcp", fmt.Sprintf("0.0.0.0:%d", socket.LocalPort))
	if err == nil {
		socket.Listener = listener
		return true
	}
	log.Println("Listner error")
	return false
}

func (socket *TCPServer) Accept() bool {
	conn, err := socket.Listener.Accept()
	if err == nil {
		socket.Socket = conn
		return true
	}
	log.Println("Accept error")
	return false
}

func (socket *TCPServer) Handshake() bool {
	// first handshake, check if client can receive and understand "MTCS Server Handshake 1"
	_, err := socket.Socket.Write([]byte(ServerHello))
	if err != nil {
		log.Println("Server First Handshake Error")
		return false
	}
	socket.ReadBuffer = make([]byte, 1056)
	socket.Socket.SetReadDeadline(time.Now().Add(5 * time.Second))
	data_length, err := socket.Socket.Read(socket.ReadBuffer)
	if err != nil || data_length <= 0 {
		log.Println("Client First Handshake Error")
		return false
	}
	if !bytes.Equal(socket.ReadBuffer[:data_length], []byte(ClientHello)) {
		log.Println("Client First Handshake Error")
		return false
	}

	// second handshake, check transport ports info with client
	portsConfirm := ""
	var portsSliceStr = make([]string, len(socket.LocalUDPPorts))
	for i := 0; i < len(socket.LocalUDPPorts); i++ {
		portsSliceStr[i] = fmt.Sprintf("%d", socket.LocalUDPPorts[i])
	}
	portsConfirm = strings.Join(portsSliceStr, ":")
	_, err = socket.Socket.Write([]byte(portsConfirm))
	if err != nil {
		log.Println("Server Second Handshake Error")
		return false
	}
	socket.Socket.SetReadDeadline(time.Now().Add(5 * time.Second))
	data_length, err = socket.Socket.Read(socket.ReadBuffer)
	if err != nil || data_length <= 0 {
		log.Println("Client Second Handshake Error")
		return false
	}

	matched := portPattern.MatchString(string(socket.ReadBuffer[:data_length]))
	if err != nil || !matched {
		log.Println("Client Second Handshake Error")
		return false
	}

	for _, v := range strings.Split(string(socket.ReadBuffer[:data_length]), ":") {
		client_port, err := strconv.ParseUint(v, 10, 16)
		if err != nil {
			log.Println("Client Second Handshake Error")
			return false
		}
		socket.ClientUDPPorts = append(socket.ClientUDPPorts, uint16(client_port))
	}

	if len(socket.ClientUDPPorts) != len(socket.LocalUDPPorts) {
		log.Println("Client Second Handshake Error")
		return false
	}
	return true
}
