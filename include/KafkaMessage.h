#ifndef KAFKAMESSAGE
#define KAFKAMESSAGE_H

#pragma once

#include <netdb.h>
#include <string_view>
#include <vector>
#include <array>
#include <cstring>

#define MIN_SUPPORTED_API_VERSION 4
#define MAX_SUPPORTED_API_VERSION 18

class KafkaMessage
{
public:
    KafkaMessage();
    ~KafkaMessage();

    struct RequestHeaderV4 {
        uint16_t api_key;
        uint16_t api_version;
        uint32_t correlation_id;
        uint16_t client_id_length;
        std::string_view client_id;
        uint8_t tag_buffer = 0;

        size_t size() const;
        void fromBuffer(const uint8_t* buffer);
    };

    struct TopicsHeaderRequestV0 {
        uint8_t topic_name_length;
        std::string_view topic_name;
        uint8_t tag_buffer;

        size_t size() const;
        void fromBuffer(const uint8_t* buffer); 
    };

    struct DescribeTopicPartitionsRequestV0 {
        uint8_t topic_array_length;
        std::vector<TopicsHeaderRequestV0> topic_array;
        uint32_t response_partition_limit;
        uint8_t cursor;
        uint8_t tag_buffer;

        size_t size() const;
        void fromBuffer(const uint8_t* buffer);
    };

    struct ResponseHeaderV0 {
        uint32_t correlation_id;

        ResponseHeaderV0(uint32_t correlation_id) :correlation_id (correlation_id){};

        size_t size() const;
        void toBuffer(uint8_t *buffer, size_t &offset) const;
    };

    struct ResponseHeaderV1 {
        uint32_t correlation_id;
        uint8_t tag_buffer;

        ResponseHeaderV1(uint32_t correlation_id, uint8_t tag_buffer) : correlation_id (correlation_id), tag_buffer (tag_buffer){}
        
        size_t size() const;
        void toBuffer(uint8_t *buffer, size_t &offset) const;
    };

    struct ApiVersionsResponse {
        uint16_t api_key;
        uint16_t min_version = htons(MIN_SUPPORTED_API_VERSION);
        uint16_t max_version = htons(MAX_SUPPORTED_API_VERSION);
        uint8_t tag_buffer;

        ApiVersionsResponse(uint16_t api_key, uint8_t tag_buffer) : api_key(api_key), tag_buffer(tag_buffer){}

        void toBuffer(uint8_t *buffer, size_t &offset) const;
    };

    struct ApiVersionResponseV4 {
        uint16_t error_code;
        uint8_t array_length;
        std::vector<ApiVersionsResponse> api_versions;
        uint32_t throttle_time_ms;
        uint8_t tag_buffer;

        void toBuffer(uint8_t *buffer, size_t &offset) const;
    };

    struct PartitionsResponse {
        uint16_t error_code;
        uint32_t partition_index;
        uint32_t leader_id;
        uint32_t leader_epoch;
        uint32_t replica_nodes;
        uint32_t isr_nodes;
        uint32_t eligible_leader_replicas;
        uint32_t last_known_elr;
        uint32_t offline_replicas;

        size_t size() const;
        void toBuffer(uint8_t *buffer, size_t &offset) const;
    };
    
    struct TopicsResponse {
        uint16_t error_code;
        uint8_t topic_name_length;
        std::string_view topic_name;
        std::array<uint8_t, 16> topic_id;
        uint8_t is_internal;
        uint8_t partition_arr_len;
        std::vector<PartitionsResponse> partitions;
        uint32_t topic_authorised_opertaions;
        uint8_t tag_buffer = 0;

        size_t size() const;
        void toBuffer(uint8_t *buffer, size_t &offset) const;
    };

    struct DescribeTopicPartitionsResponseV0 {
        uint32_t throttle_time = htonl(0);
        uint8_t array_length;
        std::vector<TopicsResponse> topics;
        uint8_t next_cursor = 0xff;
        uint8_t tag_buffer = 0;

        size_t size() const;
        void toBuffer(uint8_t *buffer, size_t &offset) const;
    };

    private:
        size_t static decodeVarint(const uint8_t* buffer, size_t& offset);
};

#endif