#ifndef MTCS_H
#define MTCS_H
#include <stdint.h>
#include <vector>
#include <cstring>
#include <stdexcept>

// multi-thread sending via UDP in any order, can recieve and assembly in any order
// after all packages send out, control thread/socket will sent a terminate packet via TCP to notice the client all packages has been sent
// then client will start to check missing packets

const uint16_t packet_data_max_length = 1024;

static const char MTCS_identifier[4] = {'M', 'T', 'C', 'S'};

uint16_t data_hash(const char *data, uint16_t length)
{
    uint32_t hash = 0;
    for (uint16_t i = 0; i < length; ++i)
    {
        hash += static_cast<uint8_t>(data[i]); // add
        hash ^= (hash << 5) | (hash >> 3);     // mix
    }
    return static_cast<uint16_t>((hash & 0xFFFF) ^ (hash >> 16));
}

#pragma pack(1)
struct MTCS
{
    // 4 bytes
    char identifier[4]; // {'M', 'T', 'C', 'S'}

    // 4 bytes ports
    uint16_t source_port;
    uint16_t target_port;

    // 2 bytes stream id
    uint16_t stream_id; // id of the current stream

    // 4 bytes full data size
    uint32_t stream_packets_count; // number of packets in current packet stream (for one sending, for next sending will need to create a new stream using same socket)

    // 2 bytes hash
    uint16_t packet_data_hash; // for data verify

    // 2 bytes packet data size
    uint16_t packet_data_length; // data length in this packet

    // 4 bytes pakcet data index
    uint32_t packet_index; // data packet index in this stream

    // 4 bytes time stamp
    uint32_t time_stamp; // when was this packet sent

    // 4 bytes service IP to forward to
    char service_ip[4];

    // 2 bytes service port to forward to
    uint16_t service_port;

    // total 32 bytes header
};

std::vector<char> MTCS_serialize(uint16_t source_port, uint16_t target_port, uint16_t stream_id, uint32_t stream_packets_count, uint16_t packet_data_length,
                                 uint32_t packet_index, uint32_t time_stamp, char *service_ip, uint16_t service_port, const char *packet_data)
{

    if (packet_data_length >= packet_data_max_length)
        throw std::runtime_error("data length excessing packet max size");

    if (packet_index > stream_packets_count - 1)
        throw std::runtime_error("data length excessing stream max size");

    std::vector<char> res = std::vector<char>(sizeof(MTCS) + packet_data_length);
    MTCS header;
    memcpy((void *)header.identifier, MTCS_identifier, sizeof(MTCS_identifier));
    header.source_port = source_port;
    header.target_port = target_port;
    header.stream_id = stream_id;
    header.stream_packets_count = stream_packets_count;
    header.packet_data_hash = data_hash(packet_data, packet_data_length);
    header.packet_data_length = packet_data_length;
    header.packet_index = packet_index;
    header.time_stamp = time_stamp;
    memcpy(res.data(), &header, sizeof(MTCS));
    memcpy(res.data() + sizeof(MTCS), packet_data, packet_data_length);
    return res;
}

// copy data as char vector
std::vector<char> MTCS_copy(const char *MTCS_packet, uint16_t MTCS_packet_length)
{
    std::vector<char> res = std::vector<char>(MTCS_packet_length);
    memcpy(res.data(), MTCS_packet, MTCS_packet_length);
    return res;
}

// get the header from char vector
MTCS MTCS_header_extract(std::vector<char> &MTCS_packet)
{
    MTCS header;
    memcpy(&header, MTCS_packet.data(), sizeof(MTCS));
    return header;
}

// parse the char vector to part of buffer
// 0 no error, 1 obvious protocol error, 2 data length not match, 3 protocol cannot identify, 4 data hash not match
int MTCS_parse(std::vector<char> MTCS_packet, char *data_buffer)
{
    // check the length of the packet, if less than header this is obvious error
    if (MTCS_packet.size() < sizeof(MTCS))
        return 1;

    MTCS header;
    memcpy(&header, MTCS_packet.data(), sizeof(MTCS));
    uint16_t packet_data_length = header.packet_data_length;
    uint32_t packet_index = header.packet_index;

    // shows the packet data is larger than 1024 bytes
    if ((packet_data_length > packet_data_max_length) || (packet_index > header.stream_packets_count - 1))
        return 1;

    // data size not matching
    if (MTCS_packet.size() - sizeof(MTCS) != header.packet_data_length)
        return 2;

    // ID not match
    if (memcmp(header.identifier, MTCS_identifier, sizeof(MTCS_identifier)) != 0)
        return 3;

    // data hash not match
    const char *packet_data_ptr = MTCS_packet.data() + sizeof(MTCS);
    if (data_hash(packet_data_ptr, packet_data_length) != header.packet_data_hash)
        return 4;

    // all correct, copy data
    memcpy(data_buffer + packet_index * packet_data_max_length, packet_data_ptr, packet_data_length);

    return 0;
}
static const uint8_t packet_header_length = sizeof(MTCS);
#endif