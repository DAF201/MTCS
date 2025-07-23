package network

import (
	"bytes"
	"fmt"
	"log"
	"math"
	"math/rand"
	"net"
	"regexp"
	"strconv"
	"strings"
	"time"
)

var portPattern = regexp.MustCompile(`^(\d{1,5})(:\d{1,5})*$`)

const (
	ServerHandShake = "MTCSSH1"
	ClientHandShake = "MTCSCH1"
)

type TCPServer struct {
	LocalPort      uint16       // port for TCP socket
	Socket         net.Conn     // Socket object
	Listener       net.Listener // Socket listener
	ReadBuffer     []byte       // Socket recv buffer
	LocalUDPPorts  []uint16     // Server Sending UDP from those ports
	ClientUDPPorts []uint16     // Client Recving UDP from those ports
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
		socket.ReadBuffer = make([]byte, 1056)
		return true
	}
	log.Println("Accept error")
	return false
}

func (socket *TCPServer) Init(localPort int, LocalUDPPorts []uint16) bool {
	socket.LocalPort = uint16(localPort)
	socket.LocalUDPPorts = LocalUDPPorts
	if !socket.Listen() {
		log.Println("Init Error: Listen Failed")
		return false
	}
	return true
}

func (socket *TCPServer) Handshake(streamContentLength uint32) (bool, uint16) {

	// first handshake, check if client can receive and understand "MTCS Server Handshake 1"
	_, err := socket.Socket.Write([]byte(ServerHandShake))
	if err != nil {
		log.Println("First Handshake Error: Server Send Failed")
		return false, 0
	}

	socket.Socket.SetReadDeadline(time.Now().Add(5 * time.Second))
	dataLength, err := socket.Socket.Read(socket.ReadBuffer)

	// check if the client can understant the server first handshake and being able to send
	if err != nil || dataLength <= 0 {
		log.Println("First Handshake Error: Server Receive Failed")
		return false, 0
	}

	// check if the reply is correct
	if !bytes.Equal(socket.ReadBuffer[:dataLength], []byte(ClientHandShake)) {
		log.Println("First Handshake Error: Client Response Cannot Be Understand")
		return false, 0
	}

	// second handshake, check transport ports info with client
	portsConfirm := ""
	var portsSliceStr = make([]string, len(socket.LocalUDPPorts))
	for i := 0; i < len(socket.LocalUDPPorts); i++ {
		portsSliceStr[i] = fmt.Sprintf("%d", socket.LocalUDPPorts[i])
	}

	// send the ports of the server want to send to
	portsConfirm = strings.Join(portsSliceStr, ":")
	_, err = socket.Socket.Write([]byte(portsConfirm))
	if err != nil {
		log.Println("Second Handshake Error: Server Send Failed")
		return false, 0
	}

	// if client does not understant and didn't reply
	socket.Socket.SetReadDeadline(time.Now().Add(5 * time.Second))
	dataLength, err = socket.Socket.Read(socket.ReadBuffer)
	if err != nil || dataLength <= 0 {
		log.Println("Second Handshake Error: Server Receive Error")
		return false, 0
	}

	// if client send back wrong data
	matched := portPattern.MatchString(string(socket.ReadBuffer[:dataLength]))
	if !matched {
		log.Println("Second Handshake Error: Client Response Not Valid")
		return false, 0
	}

	// if client send back valid UDP ports data
	for _, port := range strings.Split(string(socket.ReadBuffer[:dataLength]), ":") {
		clientPort, err := strconv.ParseUint(port, 10, 16)
		if err != nil || clientPort < 1 || clientPort > 65535 {
			log.Println("Second Handshake Error: Client Ports Not Valid")
			return false, 0
		}

		socket.ClientUDPPorts = append(socket.ClientUDPPorts, uint16(clientPort))
	}

	// if client send the wrong number of ports
	if len(socket.ClientUDPPorts) != len(socket.LocalUDPPorts) {
		log.Println("Second Handshake Error: Client Numbfer of Ports Not Valid")
		return false, 0
	}

	// third handshake, comfirm the size of transformation data and other things like Stream ID
	streamID := uint16(rand.Uint32())
	packetsCount := uint32(math.Ceil(float64(streamContentLength) / float64(1024)))
	thirdHandShakeInfo := fmt.Sprintf("%016d%016d%016d", streamID, streamContentLength, packetsCount)
	_, err = socket.Socket.Write([]byte(thirdHandShakeInfo))
	if err != nil {
		log.Println("Third Handshake Error: Server Sending Error")
		return false, 0
	}

	// if client didn't reply
	socket.Socket.SetReadDeadline(time.Now().Add(5 * time.Second))
	dataLength, err = socket.Socket.Read(socket.ReadBuffer)
	if err != nil || dataLength <= 0 {
		log.Println("Third Handshake Error: Server Recieve Error")
		return false, 0
	}

	// if client didn't repeat the content(if client confirm it can process such amount of data, it will send back the info)
	if !bytes.Equal(socket.ReadBuffer[:dataLength], []byte(thirdHandShakeInfo)) {
		log.Println("Third Handshake Error: Client Response Not Valid")
		return false, 0
	}

	// handshake success
	return true, streamID
}

func (socket *TCPServer) Write(data []byte) (length int, err error) {
	return socket.Socket.Write(data)
}

func (socket *TCPServer) Read(buffer *[]byte) (length int, err error) {
	return socket.Socket.Read(*buffer)
}

func (socket *TCPServer) Close() error {
	socket.Listener.Close()
	return socket.Socket.Close()
}
