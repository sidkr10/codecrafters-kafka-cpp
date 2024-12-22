#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

struct ClientMessage {
    u_int32_t message_size;
    u_int16_t request_api_key;
    u_int16_t request_api_version;
    u_int32_t correlation_id;
};

int main(int argc, char* argv[]) {
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket: " << std::endl;
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server_fd);
        std::cerr << "setsockopt failed: " << std::endl;
        return 1;
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

    std::cout << "Waiting for a client to connect...\n";

    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!\n";
    
    // Uncomment this block to pass the first stage
    
    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
    std::cout << "Client connected\n";
    
    // read from client
    
    int n;
    u_int8_t buffer[256];
    n = read(client_fd, &buffer, sizeof(buffer));

    if(n > 0) {
        int msg_size = 4;
        int cid;
        ClientMessage clientMsg;
        size_t offset = 0;
        clientMsg.message_size = (buffer[offset] << 24) | (buffer[offset + 1] << 16) |
                         (buffer[offset + 2] << 8) | buffer[offset + 3];
        offset += 4;
        clientMsg.request_api_key = (buffer[offset] << 8) | buffer[offset +1];
        offset += 2;
        clientMsg.request_api_version = (buffer[offset] << 8) | buffer[offset +1];
        offset += 2;
        clientMsg.correlation_id = (buffer[offset] << 24) | (buffer[offset + 1] << 16) |
                         (buffer[offset + 2] << 8) | buffer[offset + 3];
        offset += 4;
        
        // write response message to client
        clientMsg.message_size = htonl(clientMsg.message_size);
        clientMsg.correlation_id = htonl(clientMsg.correlation_id);
        write(client_fd, &clientMsg.message_size, sizeof(clientMsg.message_size));
        write(client_fd, &clientMsg.correlation_id, sizeof(clientMsg.correlation_id));

    } else {
        std::cout << "No message received from client\n"; 
    }

    close(client_fd);

    close(server_fd);
    return 0;
}