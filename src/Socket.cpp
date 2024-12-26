#include "Socket.h"

Socket::Socket() {}

Socket::~Socket()
{
    if (server_fd != -1)
    {
        std::cerr << "Closing Socket Connection\n";
        close(server_fd);
    }
}

void Socket::RequestMessage::fromBuffer(const u_int8_t *buffer, size_t buffer_size)
{
    if (buffer_size < sizeof(RequestMessage))
    {
        std::cerr << "Client message size is invalid\n";
    }
    request_api_key = ntohs(*reinterpret_cast<const u_int16_t *>(buffer));
    request_api_version = ntohs(*reinterpret_cast<const u_int16_t *>(buffer + 2));
    correlation_id = ntohl(*reinterpret_cast<const u_int32_t *>(buffer + 4));
    std::cerr << message_size << " " << correlation_id << "\n";
}

void Socket::ApiVersion::toBuffer(std::vector<uint8_t> &buffer) const
{
    size_t offset = buffer.size();
    buffer.resize(offset + 7);
    memcpy(buffer.data() + offset, &api_key, sizeof(uint16_t));
    offset += sizeof(api_key);
    memcpy(buffer.data() + offset, &min_supported_version, sizeof(uint16_t));
    offset += sizeof(min_supported_version);
    memcpy(buffer.data() + offset, &max_supported_version, sizeof(uint16_t));
    offset += sizeof(max_supported_version);
    memcpy(buffer.data() + offset, &tag_buffer, sizeof(uint8_t));
}

void Socket::ApiVersionsResponseBody::toBuffer(std::vector<uint8_t> &buffer) const
{
    size_t offset = buffer.size();
    buffer.resize(offset + sizeof(uint16_t) + sizeof(uint8_t));
    memcpy(buffer.data() + offset, &error_code, sizeof(uint16_t));
    offset += sizeof(error_code);
    memcpy(buffer.data() + offset, &api_arr_len, sizeof(uint8_t));

    for (const auto &version : api_versions)
    {
        version.toBuffer(buffer);
        offset = buffer.size();
    }

    buffer.resize(offset + sizeof(uint32_t) + sizeof(uint8_t));
    memcpy(buffer.data() + offset, &throttle_time, sizeof(uint32_t));
    offset += sizeof(throttle_time);
    memcpy(buffer.data() + offset, &tag_buffer, sizeof(uint8_t));
}

void Socket::ResponseMessage::toBuffer(std::vector<u_int8_t> &buffer) const
{
    size_t offset = buffer.size() + 4;
    buffer.resize(sizeof(uint32_t) + sizeof(uint32_t));
    memcpy(buffer.data() + offset, &correlation_id, sizeof(uint32_t));
    response_body.toBuffer(buffer);
}

int Socket::createSocket()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Failed to create server socket: " << std::endl;
        return -1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        close(server_fd);
        std::cerr << "setsockopt failed: " << std::endl;
        return -1;
    }

    struct sockaddr_in server_addr
    {
    };
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9092);

    if (bind(server_fd, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) != 0)
    {
        close(server_fd);
        std::cerr << "Failed to bind to port 9092" << std::endl;
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0)
    {
        close(server_fd);
        std::cerr << "listen failed" << std::endl;
        return 1;
    }

    std::cerr << "Waiting for a client to connect...\n";

    return server_fd;
}

int Socket::acceptConnection()
{
    struct sockaddr_in client_addr
    {
    };
    socklen_t client_addr_len = sizeof(client_addr);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    // std::cerr << "Logs from your program will appear here!\n";

    // Uncomment this block to pass the first stage

    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_len);
    std::cerr << "Client connected\n";

    return client_fd;
}

Socket::RequestMessage Socket::readBufferFromClient(int client_fd)
{
    uint32_t message_len;
    int n = read(client_fd, &message_len, sizeof(message_len));
    RequestMessage requestMessage;
    size_t buffer_size = ntohl(message_len);
    if(n <= 0) {
        requestMessage.message_size = 0;
        return requestMessage;
    }
    requestMessage.message_size = buffer_size;
    ssize_t bytes_read;
    u_int8_t buffer[buffer_size];
    bytes_read = recv(client_fd, &buffer, buffer_size,0);

    if(bytes_read != buffer_size) {
        requestMessage.message_size = 0;
    }
    requestMessage.fromBuffer(buffer,buffer_size);
    return requestMessage;
}

void Socket::writeBufferToClient(int client_fd, ResponseMessage &responseMessage)
{
    std::vector<uint8_t> buffer;
    buffer.reserve(1024);
    size_t offset = buffer.size();
    uint8_t api_arr_len = responseMessage.response_body.api_versions.size() + 1;
    responseMessage.response_body.api_arr_len = api_arr_len;
    responseMessage.toBuffer(buffer);
    responseMessage.message_size = htonl(buffer.size() - 4);
    memcpy(buffer.data() + offset, &(responseMessage.message_size), sizeof(uint32_t));
    ssize_t bytes_sent = send(client_fd, buffer.data(), buffer.size(), 0);
    
}