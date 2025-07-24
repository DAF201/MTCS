package protocol

import (
	"MTCS/types"
	"bytes"
	"encoding/binary"
	"time"
)

type MTCS struct {
	Identifier         [4]byte
	SourcePort         types.Uint16_t
	TargetPort         types.Uint16_t
	StreamID           types.Uint16_t
	ChannelID          types.Uint8_t
	ResendAttempt      types.Uint8_t
	StreamPacketsCount types.Uint32_t
	PacketDataHash     types.Uint16_t
	PacketDataLength   types.Uint16_t
	PacketIndex        types.Uint32_t
	Timestamp          types.Uint32_t
	Reserved           [4]byte
}

func MTCSSerialize(
	SourcePort, TargetPort, StreamID types.Uint16_t,
	ChannelID, ResendAttempt types.Uint8_t,
	StreamPacketsCount types.Uint32_t,
	PacketDataHash, PacketDataLength types.Uint16_t,
	PacketIndex types.Uint32_t,
	data []byte) types.Bytes {
	var res types.Bytes
	var header MTCS
	header.Identifier = [4]byte{'M', 'T', 'C', 'S'}
	header.SourcePort = SourcePort
	header.TargetPort = TargetPort
	header.StreamID = StreamID
	header.ChannelID = ChannelID
	header.ResendAttempt = ResendAttempt
	header.StreamPacketsCount = StreamPacketsCount
	header.PacketDataHash = PacketDataHash
	header.PacketDataLength = PacketDataLength
	header.PacketIndex = PacketIndex
	var ts types.Uint32_t
	binary.LittleEndian.PutUint32(ts[:], uint32(time.Now().Unix()))
	header.Timestamp = ts
	headerData, err := types.StructToBytes(header)
	if err == nil {
		res = append(res, headerData...)
	} else {
		panic("failed to convert header to bytes")
	}
	res = append(res, data...)
	return res
}

func MTCSParse(data types.Bytes) (MTCS, types.Bytes) {
	var MTCS_header MTCS
	reader := bytes.NewReader(data[:32])
	err := binary.Read(reader, binary.LittleEndian, &MTCS_header)
	if err != nil {
		panic("Failed to read MTCS header: " + err.Error())
	}
	packet_content := data[32:]
	return MTCS_header, packet_content
}
