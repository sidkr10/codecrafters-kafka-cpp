#include "Socket.h"

Socket::Socket(){}

Socket::~Socket()
{
    if(server_fd != -1){
        std::cerr << "Closing Socket Connection\n";
        close(server_fd);
    }
}

void Socket::RequestMessage::fromBuffer(const u_int8_t *buffer, size_t buffer_size){
    if(buffer_size < sizeof(RequestMessage)){
        std::cerr << "Client message size is invalid\n";
    }
    message_size = ntohl(*reinterpret_cast<const u_int32_t *>(buffer));
    request_api_key = ntohs(*reinterpret_cast<const u_int16_t *>(buffer + 4));
    request_api_version = ntohs(*reinterpret_cast<const u_int16_t *>(buffer + 6));
    correlation_id = ntohl(*reinterpret_cast<const u_int32_t *>(buffer + 8));
}

u_int8_t* Socket::ResponseMessage::toBuffer(){
    u_int8_t *buffer = new u_int8_t[32];

    uint8_t num_api_keys = 2;
    int16_t api_key = htons(18);
    uint8_t tag_buffer = 0;
    int32_t throttle_time_ms = htonl(0);

    *reinterpret_cast<uint32_t *>(buffer) = htonl(sizeof(int32_t) * 2 + sizeof(int16_t) * 4 + sizeof(uint8_t) * 3);
    *reinterpret_cast<int32_t *>(buffer + 4) = htonl(correlation_id);
    *reinterpret_cast<uint16_t *>(buffer + 8) = htons(error_code);
    *reinterpret_cast<uint8_t *>(buffer + 10) = num_api_keys;
    *reinterpret_cast<uint16_t *>(buffer + 11) = api_key;
    *reinterpret_cast<uint16_t *>(buffer + 13) = htons(MIN_SUPPORTED_API_VERSION);
    *reinterpret_cast<uint16_t *>(buffer + 15) = htons(MAX_SUPPORTED_API_VERSION);
    *reinterpret_cast<uint8_t *>(buffer + 17) = tag_buffer;
    *reinterpret_cast<uint32_t *>(buffer + 18) = throttle_time_ms;
    *reinterpret_cast<uint8_t *>(buffer + 22) = tag_buffer;
    
    return buffer;
}

int Socket::createSocket(){
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket: " << std::endl;
        return -1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server_fd);
        std::cerr << "setsockopt failed: " << std::endl;
        return -1;
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9092);

    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        close(server_fd);
        std::cerr << "Failed to bind to port 9092" << std::endl;
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        close(server_fd);
        std::cerr << "listen failed" << std::endl;
        return 1;
    }

    std::cerr << "Waiting for a client to connect...\n";

    return server_fd;
}

int Socket::acceptConnection(){
    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!\n";
    
    // Uncomment this block to pass the first stage
    
    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
    std::cerr << "Client connected\n";

    return client_fd;
}

Socket::RequestMessage Socket::readBufferFromClient(int client_fd){
    RequestMessage requestMessage;
    constexpr size_t buffer_size = sizeof(RequestMessage);
    int n;
    u_int8_t buffer[buffer_size];
    n = read(client_fd, &buffer, sizeof(buffer));

    if(n > 0) {
        requestMessage.fromBuffer(buffer, sizeof(buffer));
    }

    return requestMessage;
}

void Socket::writeBufferToClient(int client_fd, Socket::ResponseMessage &responseMessage){
    const u_int8_t *buffer = responseMessage.toBuffer();
    const size_t buffer_size = 32;
    int n = write(client_fd, buffer, buffer_size);
    sleep(1);
    if(n > 0){
        close(client_fd);
    }
    delete[] buffer;
}