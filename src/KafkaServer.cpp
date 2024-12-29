#include "KafkaServer.h"

KafkaServer::KafkaServer()
{
    server_fd = socket.createSocket();
}

KafkaServer::~KafkaServer()
{

}

int KafkaServer::acceptConnections()
{    
    // Wait for connection from client and accept
    return socket.acceptConnection();
}

int KafkaServer::handleClient(int client_fd)
{
    if(server_fd <= 0){
        std::cerr << "Server has not started. Please start the server to handle clients";
        return -1;
    }
    if(client_fd <= 0){
        std::cerr << "Client connection is not established or is closed.";
        return -1;
    }
    while (1)
    {
        // Once Connection established read the message from client
        const uint8_t* request = socket.readBufferFromClient(client_fd);

        if (request == 0)
            break;

        // Fetch Request Header
        size_t offset = 0;
        KafkaMessage::RequestHeaderV4 requestHeader;
        requestHeader.fromBuffer(request);

        uint8_t* response = new uint8_t[1024];
        switch(requestHeader.api_key){
            case 18 :
                offset = (this->processApiVersionsReqeuest(request,response));
                break;
            case 75 :
                offset = (this->processDescribeTopicPartitionsRequest(request, response));
                break;
        }
        socket.writeBufferToClient(client_fd, response, offset);
        
        delete[] request;
        delete[] response;
    }
    return 1;
}

size_t KafkaServer::processApiVersionsReqeuest(const uint8_t *requestMessage, uint8_t* response)
{
    size_t offset = 0;
    // Fetch Request Header
    KafkaMessage::RequestHeaderV4 requestHeader;
    requestHeader.fromBuffer(requestMessage + offset);
    offset += requestHeader.size();

    KafkaMessage::ResponseHeaderV0 responseHeader(requestHeader.correlation_id);
    
    KafkaMessage::ApiVersionsResponse apiVersion1(requestHeader.api_key, 0);
    KafkaMessage::ApiVersionResponseV4 apiVersionResponse;
    apiVersionResponse.api_versions.push_back(apiVersion1);

    KafkaMessage::ApiVersionsResponse apiVersion2(requestHeader.api_key, 0);
    apiVersionResponse.api_versions.push_back(apiVersion2);
    
    KafkaMessage::ApiVersionsResponse apiVersion3(75, 0);
    apiVersion3.min_version = htons(0);
    apiVersion3.max_version = htons(4);
    apiVersionResponse.api_versions.push_back(apiVersion3);

    apiVersionResponse.error_code = (requestHeader.api_version < 0 || requestHeader.api_version > 4) ? UNSUPPORTED_VERSION : 0;
    apiVersionResponse.array_length = apiVersionResponse.api_versions.size() + 1;

    apiVersionResponse.throttle_time_ms = 0;
    apiVersionResponse.tag_buffer = 0;

    offset = 4;
    responseHeader.toBuffer(response, offset);
    apiVersionResponse.toBuffer(response, offset);

    
    uint32_t msg_size = htonl(offset - 4);
    memcpy(response, &msg_size, sizeof(msg_size));

    return offset;
}

size_t KafkaServer::processDescribeTopicPartitionsRequest(const uint8_t *requestMessage, uint8_t* response)
{
    size_t offset = 0;

    // Fetch Response Header
    KafkaMessage::RequestHeaderV4 requestHeader;
    requestHeader.fromBuffer(requestMessage + offset);
    offset += requestHeader.size();

    KafkaMessage::DescribeTopicPartitionsRequestV0 describeTopicPartitionRequest;
    describeTopicPartitionRequest.fromBuffer(requestMessage + offset);
    offset += describeTopicPartitionRequest.size();

    // Fetch Topic Header Request
    KafkaMessage::TopicsHeaderRequestV0 topicRequest = describeTopicPartitionRequest.topic_array[0];
    
    // Create a response
    std::memset(response, 0, 1024);  // Zero initialize the buffer
    
    // Create response header with same correlation id
    KafkaMessage::ResponseHeaderV1 responseHeader(requestHeader.correlation_id, 0);
    
    // Create response body with error code UNKNOWN_TOPIC_OR_PARTITION
    KafkaMessage::DescribeTopicPartitionsResponseV0 describeTopicResponseBody;
    KafkaMessage::TopicsResponse topicResponse1;
    memset(&topicResponse1, 0, sizeof(KafkaMessage::TopicsResponse));
    topicResponse1.error_code = UNKNOWN_TOPIC_OR_PARTITION;
    topicResponse1.topic_name_length = topicRequest.topic_name_length;
    topicResponse1.topic_name = topicRequest.topic_name;
    topicResponse1.topic_id.fill(0);
    topicResponse1.partition_arr_len = topicResponse1.partitions.size() + 1;
    describeTopicResponseBody.topics.push_back(topicResponse1);
    
    offset = 4;
    responseHeader.toBuffer(response, offset);
    describeTopicResponseBody.toBuffer(response, offset);
    
    uint32_t msg_size = htonl(offset - 4);
    memcpy(response, &msg_size, sizeof(msg_size));
            
    return offset;
}
