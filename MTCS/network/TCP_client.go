package network

import (
	"bytes"
	"fmt"
	"log"
	"net"
	"strconv"
	"strings"
	"time"
)

type TCPClient struct {
	RemoteAddress  string   // server IP
	RemotePort     uint16   // server port
	LocalTCPPort   uint16   // local port
	Socket         net.Conn // Socket object
	ReadBuffer     []byte   // Client recv buffer
	LocalUDPPorts  []uint16 // Client UDP from those ports
	ServerUDPPorts []uint16 // Server UDP from those ports
}

func (socket *TCPClient) Connect(remoteAddress string, remotePort int) {

}

func (socket *TCPClient) Init(remoteAddress string, remotePort int, LocalTCPPort int, localUDPPorts []uint16) bool {
	socket.LocalTCPPort = uint16(LocalTCPPort)
	conn, err := net.Dial("tcp", fmt.Sprintf("%s:%s", socket.RemoteAddress, socket.RemotePort))
	if err != nil {
		log.Println("Connection Error: Cannot Connect To Server")
		return false
	}
	socket.RemoteAddress = remoteAddress
	socket.RemotePort = uint16(remotePort)
	socket.LocalTCPPort = uint16(LocalTCPPort)
	socket.LocalUDPPorts = localUDPPorts
	socket.Socket = conn
	return true
}

func (socket *TCPClient) Handshake() (bool, uint16) {
	dataLength, err := socket.Socket.Read(socket.ReadBuffer)
	if err != nil || dataLength <= 0 {
		log.Println("First Handshake Error: Client Read Failed")
		return false, 0
	}

	if !bytes.Equal(socket.ReadBuffer[:dataLength], []byte(ServerHandShake)) {
		log.Println("First Handshake Error: Server Response Cannot Be Understand")
		return false, 0
	}

	_, err = socket.Socket.Write([]byte(ClientHandShake))
	if err != nil {
		log.Println("First Handshake Error: Client Send Failed")
		return false, 0
	}

	socket.Socket.SetReadDeadline(time.Now().Add(5 * time.Second))
	dataLength, err = socket.Socket.Read(socket.ReadBuffer)
	if err != nil {
		log.Println("Second Handshake Error: Client Receive Failed")
		return false, 0
	}

	matched := portPattern.MatchString(string(socket.ReadBuffer[:dataLength]))
	if !matched {
		log.Println("Second Handshake Error: Server Response Not Valid")
		return false, 0
	}

	// if server send back valid UDP ports data
	dataLength, err = socket.Socket.Read(socket.ReadBuffer)
	if dataLength <= 0 || err != nil {
		log.Println("Second Handshake Error: Client Receive Failed")
		return false, 0
	}

	for _, port := range strings.Split(string(socket.ReadBuffer[:dataLength]), ":") {
		serverPort, err := strconv.ParseUint(port, 10, 16)
		if err != nil || serverPort < 1 || serverPort > 65535 {
			log.Println("Second Handshake Error: Client Ports Not Valid")
			return false, 0
		}
		socket.ServerUDPPorts = append(socket.ServerUDPPorts, uint16(serverPort))
	}

	portsConfirm := ""
	var portsSliceStr = make([]string, len(socket.LocalUDPPorts))
	for i := 0; i < len(portsSliceStr); i++ {
		portsSliceStr[i] = string(socket.LocalUDPPorts[i])
	}

	portsConfirm = strings.Join(portsSliceStr, ":")
	_, err = socket.Socket.Write([]byte(portsConfirm))
	if err != nil {
		log.Println("Second Handshake Error: Client Send Failed")
		return false, 0
	}

	if len(socket.ServerUDPPorts) != len(socket.LocalUDPPorts) {
		log.Println("Second Handshake Error: Client Numbfer of Ports Not Valid")
		return false, 0
	}

	// third handshake, comfirm the size of transformation data and other things like Stream ID
	dataLength, err = socket.Socket.Read(socket.ReadBuffer)
	if err != nil || dataLength <= 0 {
		log.Println("Third Handshake Error: Client Recieve Error")
		return false, 0
	}

	streamID64, err := strconv.ParseUint(strings.TrimLeft(string(socket.ReadBuffer), "0"), 10, 16)
	if err != nil {
		log.Println("Third Handshake Error: Server Response Not Valid")
		return false, 0
	}
	streamID := uint16(streamID64)

	_, err = socket.Socket.Write(socket.ReadBuffer)

	return true, streamID
}
