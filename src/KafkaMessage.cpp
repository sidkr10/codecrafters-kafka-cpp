#include "KafkaMessage.h"

KafkaMessage::KafkaMessage()
{

}

KafkaMessage::~KafkaMessage()
{

}

size_t KafkaMessage::decodeVarint(const uint8_t* buffer, size_t& offset) {
    size_t value = 0;
    int shift = 0;

    while (true) {
        uint8_t byte = buffer[offset++];
        value |= (byte & 0x7F) << shift;
        
        if ((byte & 0x80) == 0)  // Stop if MSB is 0
            break;

        shift += 7;
    }

    return value;
}

size_t KafkaMessage::RequestHeaderV4::size() const
{
    if (client_id_length == 0)
        return sizeof(sizeof(api_key) + sizeof(api_version) + sizeof(correlation_id) + sizeof(client_id_length));
    return sizeof(api_key) + sizeof(api_version) + sizeof(correlation_id) + sizeof(client_id_length) + client_id.size() + sizeof(tag_buffer);
}

void KafkaMessage::RequestHeaderV4::fromBuffer(const uint8_t* buffer)
{
    api_key = ntohs(*reinterpret_cast<const uint16_t*>(buffer));
    api_version = ntohs(*reinterpret_cast<const uint16_t*>(buffer + 2));
    correlation_id = ntohl(*reinterpret_cast<const uint32_t*>(buffer + 4));
    client_id_length = ntohs(*reinterpret_cast<const uint16_t*>(buffer + 8));
    if(client_id_length != static_cast<uint16_t>(-1))
        client_id = std::string_view(reinterpret_cast<const char*>(buffer + 10), client_id_length);
    else
        client_id = std::string_view();
}

size_t KafkaMessage::TopicsHeaderRequestV0::size() const
{
    if(topic_name_length <= 1)
        return sizeof(sizeof(topic_name_length) + sizeof(tag_buffer));
    return sizeof(topic_name_length) + topic_name.size() + sizeof(tag_buffer) - 1; // -1 because topic_name is c-style string
}

void KafkaMessage::TopicsHeaderRequestV0::fromBuffer(const uint8_t* buffer)
{
    topic_name_length = buffer[0];
     if (topic_name_length > 0) {
        topic_name = std::string_view(reinterpret_cast<const char*>(buffer + 1), topic_name_length);
    } else {
        topic_name = std::string_view();  // Empty string view for 0 length
    }
    tag_buffer = buffer[1 + topic_name_length];  // Tag buffer right after the topic name

}

size_t KafkaMessage::DescribeTopicPartitionsRequestV0::size() const
{
    if(topic_array_length <= 1)
        return sizeof(sizeof(topic_array_length) + sizeof(response_partition_limit) + sizeof(cursor) + sizeof(tag_buffer));
    size_t topic_size = 0;
    for(TopicsHeaderRequestV0 topic : topic_array){
        topic_size += topic.size();
    }
    return sizeof(sizeof(topic_array_length) + sizeof(response_partition_limit) + sizeof(cursor) + sizeof(tag_buffer) + topic_size);
}

void KafkaMessage::DescribeTopicPartitionsRequestV0::fromBuffer(const uint8_t* buffer)
{
    size_t offset = 0;

    // Decode topic array length (array length + 1 encoded as varint)
    size_t encoded_array_length = decodeVarint(buffer, offset);  
    topic_array_length = encoded_array_length;

    topic_array.clear();
    for (size_t i = 0; i < topic_array_length -1; ++i) {
        TopicsHeaderRequestV0 topic;
        topic.fromBuffer(buffer + offset);
        offset += topic.size();  // Move offset
        topic_array.push_back(topic);
    }

    response_partition_limit = ntohl(*reinterpret_cast<const uint32_t*>(buffer + offset));
    offset += sizeof(uint32_t);

    cursor = buffer[offset++];
    tag_buffer = buffer[offset];
}

size_t KafkaMessage::ResponseHeaderV1::size() const
{
    return sizeof(correlation_id) + sizeof(tag_buffer);;
}

void KafkaMessage::ResponseHeaderV1::toBuffer(uint8_t *buffer, size_t &offset) const
{
    // Write correlation_id (convert to network byte order)
    uint32_t net_correlation_id = htonl(correlation_id);
    std::memcpy(buffer + offset, &net_correlation_id, sizeof(net_correlation_id));
    offset += sizeof(net_correlation_id);

    // Write tag_buffer
    buffer[offset++] = tag_buffer;
}

void KafkaMessage::ApiVersionsResponse::toBuffer(uint8_t *buffer, size_t &offset) const
{
    // Write api_key (convert to network byte order)
    uint16_t net_api_key = htons(api_key);
    std::memcpy(buffer + offset, &net_api_key, sizeof(net_api_key));
    offset += sizeof(net_api_key);

    // Write min_version and max_version (already in network byte order)
    std::memcpy(buffer + offset, &min_version, sizeof(min_version));
    offset += sizeof(min_version);

    std::memcpy(buffer + offset, &max_version, sizeof(max_version));
    offset += sizeof(max_version);

    // Write tag_buffer
    buffer[offset++] = tag_buffer;
}

void KafkaMessage::ApiVersionResponseV4::toBuffer(uint8_t *buffer, size_t &offset) const
{
    // Write error_code (convert to network byte order)
    uint16_t net_error_code = htons(error_code);
    std::memcpy(buffer + offset, &net_error_code, sizeof(net_error_code));
    offset += sizeof(net_error_code);

    // Write array_length
    buffer[offset++] = array_length;

    // Write each ApiVersionsResponse to buffer
    for (const auto& version : api_versions) {
        version.toBuffer(buffer, offset);
    }

    // Write throttle_time_ms (convert to network byte order)
    uint32_t net_throttle_time = htonl(throttle_time_ms);
    std::memcpy(buffer + offset, &net_throttle_time, sizeof(net_throttle_time));
    offset += sizeof(net_throttle_time);

    buffer[offset++] = tag_buffer;
}

size_t KafkaMessage::PartitionsResponse::size() const
{
    return sizeof(error_code) +
           sizeof(partition_index) +
           sizeof(leader_id) +
           sizeof(leader_epoch) +
           sizeof(replica_nodes) +
           sizeof(isr_nodes) +
           sizeof(eligible_leader_replicas) +
           sizeof(last_known_elr) +
           sizeof(offline_replicas);
}

void KafkaMessage::PartitionsResponse::toBuffer(uint8_t *buffer, size_t &offset) const
{
    // Write error_code
    uint16_t net_error_code = htons(error_code);
    std::memcpy(buffer + offset, &net_error_code, sizeof(net_error_code));
    offset += sizeof(net_error_code);

    // Write partition_index
    uint32_t net_partition_index = htonl(partition_index);
    std::memcpy(buffer + offset, &net_partition_index, sizeof(net_partition_index));
    offset += sizeof(net_partition_index);

    // Write leader_id
    uint32_t net_leader_id = htonl(leader_id);
    std::memcpy(buffer + offset, &net_leader_id, sizeof(net_leader_id));
    offset += sizeof(net_leader_id);

    // Write leader_epoch
    uint32_t net_leader_epoch = htonl(leader_epoch);
    std::memcpy(buffer + offset, &net_leader_epoch, sizeof(net_leader_epoch));
    offset += sizeof(net_leader_epoch);

    // Write replica_nodes
    uint32_t net_replica_nodes = htonl(replica_nodes);
    std::memcpy(buffer + offset, &net_replica_nodes, sizeof(net_replica_nodes));
    offset += sizeof(net_replica_nodes);

    // Write isr_nodes
    uint32_t net_isr_nodes = htonl(isr_nodes);
    std::memcpy(buffer + offset, &net_isr_nodes, sizeof(net_isr_nodes));
    offset += sizeof(net_isr_nodes);

    // Write eligible_leader_replicas
    uint32_t net_eligible_leader_replicas = htonl(eligible_leader_replicas);
    std::memcpy(buffer + offset, &net_eligible_leader_replicas, sizeof(net_eligible_leader_replicas));
    offset += sizeof(net_eligible_leader_replicas);

    // Write last_known_elr
    uint32_t net_last_known_elr = htonl(last_known_elr);
    std::memcpy(buffer + offset, &net_last_known_elr, sizeof(net_last_known_elr));
    offset += sizeof(net_last_known_elr);

    // Write offline_replicas
    uint32_t net_offline_replicas = htonl(offline_replicas);
    std::memcpy(buffer + offset, &net_offline_replicas, sizeof(net_offline_replicas));
    offset += sizeof(net_offline_replicas);
}

size_t KafkaMessage::TopicsResponse::size() const
{
    size_t total_size = sizeof(error_code) +
                        sizeof(topic_name_length) +
                        topic_name.size() +            // Size of the topic name string
                        topic_id.size() +              // Fixed 16 bytes for topic_id
                        sizeof(is_internal) +
                        sizeof(topic_authorised_opertaions) +
                        sizeof(tag_buffer);            // 1 byte for tag_buffer

    // Add size of all partitions
    for (const auto& partition : partitions) {
        total_size += partition.size();
    }

    return total_size;
}

void KafkaMessage::TopicsResponse::toBuffer(uint8_t *buffer, size_t &offset) const
{
    // Write error_code
    uint16_t net_error_code = htons(error_code);
    std::memcpy(buffer + offset, &net_error_code, sizeof(net_error_code));
    offset += sizeof(net_error_code);

    // Write topic_name_length
    buffer[offset++] = topic_name_length;

    // Write topic_name (length bytes directly into buffer)
    std::memcpy(buffer + offset, topic_name.data(), topic_name.size());
    offset += topic_name.size();

    // Write topic_id
    std::memcpy(buffer + offset, topic_id.data(), topic_id.size());
    offset += topic_id.size();

    // Write is_internal
    buffer[offset++] = is_internal;

    // Write partitions array
    // uint8_t partitions_count = static_cast<uint8_t>(partitions.size() + 1);
    // buffer[offset++] = partitions_count;
    // for (const auto &partition : partitions) {
    //     partition.toBuffer(buffer, offset);
    // }

    // Write topic_authorised_opertaions
    uint32_t net_topic_authorised_opertaions = htonl(topic_authorised_opertaions);
    std::memcpy(buffer + offset, &net_topic_authorised_opertaions, sizeof(net_topic_authorised_opertaions));
    offset += sizeof(net_topic_authorised_opertaions);

    // Write tag_buffer
    buffer[offset++] = tag_buffer;
}

size_t KafkaMessage::DescribeTopicPartitionsResponseV0::size() const
{
    size_t total_size = sizeof(throttle_time) +
                        sizeof(array_length) +
                        sizeof(next_cursor) +
                        sizeof(tag_buffer);

    // Add size of all topics
    for (const auto& topic : topics) {
        total_size += topic.size();
    }

    return total_size;
}

void KafkaMessage::DescribeTopicPartitionsResponseV0::toBuffer(uint8_t *buffer, size_t &offset) const
{
     // Write throttle_time (already in network byte order)
    std::memcpy(buffer + offset, &throttle_time, sizeof(throttle_time));
    offset += sizeof(throttle_time);

    // Write array_length
    buffer[offset++] = topics.size() + 1;

    // Write topics array
    for (const auto &topic : topics) {
        topic.toBuffer(buffer, offset);
    }

    // Write next_cursor
    buffer[offset++] = next_cursor;

    // Write tag_buffer
    buffer[offset++] = tag_buffer;
}

size_t KafkaMessage::ResponseHeaderV0::size() const
{
    return sizeof(correlation_id);
}

void KafkaMessage::ResponseHeaderV0::toBuffer(uint8_t *buffer, size_t &offset) const
{
    uint32_t net_correlation_id = htonl(correlation_id);
    memcpy(buffer + offset, &net_correlation_id, sizeof(net_correlation_id));
    offset += sizeof(net_correlation_id);
}
