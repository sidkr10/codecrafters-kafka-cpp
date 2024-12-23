#include <cstdlib>
#include <cstring>
#include <vector>
#include <Socket.h>

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

    Socket socket;

    // Create socket and start listening for client
    int server_fd = socket.createSocket();

    // Wait for connection from client and accept
    int client_fd = socket.acceptConnection();

    // Once Connection established read the message from client
    Socket::RequestMessage requestMessage = socket.readBufferFromClient(client_fd);    
    
    // Create a response message with request correlation id
    Socket::ResponseMessage responseMessage;
    responseMessage.message_size = requestMessage.message_size;
    responseMessage.request_api_version = requestMessage.request_api_version;
    responseMessage.correlation_id = requestMessage.correlation_id;

    // Write back to client    
    socket.writeBufferToClient(client_fd, responseMessage);
    
    return 0;
}