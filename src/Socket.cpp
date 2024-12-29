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

uint8_t* Socket::readBufferFromClient(int client_fd)
{
    uint32_t message_len;
    int n = read(client_fd, &message_len, sizeof(message_len));
    uint8_t* requestMessage;
    if(n <= 0) {
        return 0;
    }
    size_t buffer_size = ntohl(message_len);
    requestMessage = new uint8_t[buffer_size];
    size_t bytes_read;
    bytes_read = recv(client_fd, requestMessage, buffer_size,0);

    if(bytes_read != buffer_size) {
        std::cerr << "Request Body does not match the message size!\n";
        delete[] requestMessage;
        return 0;
    }
    
    return requestMessage;
}

void Socket::writeBufferToClient(int client_fd, const uint8_t *buffer, size_t &buffer_size)
{
    size_t bytes_sent = send(client_fd, buffer, buffer_size, 0);
    if(bytes_sent != buffer_size){
        std::cerr << "Error when sending message.\n";
    }
}