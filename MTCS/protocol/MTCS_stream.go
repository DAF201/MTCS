package protocol

import (
	"MTCS/types"
	"math"
	"math/rand"
	"sync/atomic"
)

const MTCS_packet_header_length uint8 = 32
const MTCS_packet_max_content_length uint16 = 1024

// numbers of sub threads, each first request a unique packet index from the stream, then send out the corresponding part of data

type MTCSStream struct {
	StreamID           uint16         // save ID of current stream
	StreamPacketsCount uint32         // number of packets in this stream
	PacketIndex        uint32         // index of the next packet to send out
	Packets            []*types.Bytes // packets content
}

// get an unique index, this operation is atomic so no threads will get the same index
func (stream *MTCSStream) GetPacketIndex() uint32 {
	return uint32(atomic.AddUint32(&stream.PacketIndex, 1) - 1)
}

// build a MTCS Stream, and each sub thread will get the pointer of it to get unique index from
func BuildMTCSStream(num_of_channels int, full_data types.Bytes) MTCSStream {

	var stream MTCSStream
	packets_count := uint32(math.Ceil(float64(len(full_data)) / float64(MTCS_packet_max_content_length))) //calculate how many packets are needed
	stream.StreamPacketsCount = packets_count

	stream.StreamID = uint16(rand.Int31n(65536)) //create a stream id

	stream.PacketIndex = 0 //sending index

	stream.Packets = make([]*types.Bytes, packets_count)

	for i := 0; i < int(packets_count); i++ {
		start := i * int(MTCS_packet_max_content_length)
		end := start + int(MTCS_packet_max_content_length)
		if end > len(full_data) {
			end = len(full_data)
		}
		chunk := full_data[start:end]
		copyChunk := make(types.Bytes, len(chunk)+int(MTCS_packet_header_length))
		copy(copyChunk, chunk)
		stream.Packets[i] = &copyChunk
	}
	return stream
}
