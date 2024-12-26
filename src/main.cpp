#include <cstdlib>
#include <cstring>
#include <vector>
#include <Socket.h>

int main(int argc, char *argv[])
{
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    Socket socket;

    // Create socket and start listening for client
    int server_fd = socket.createSocket();
    int client_fd = -1;
    
    // Wait for connection from client and accept
    client_fd = socket.acceptConnection();
    while (1)
    {
        // Once Connection established read the message from client
        Socket::RequestMessage requestMessage = socket.readBufferFromClient(client_fd);

        if (requestMessage.message_size == 0)
            break;

        // Create a response message with request correlation id
        Socket::ResponseMessage responseMessage;
        responseMessage.correlation_id = htonl(requestMessage.correlation_id);

        Socket::ApiVersionsResponseBody responseBody;

        Socket::ApiVersion api_version1;
        api_version1.api_key = htons(18);

        responseBody.api_versions.push_back(api_version1);
        responseBody.error_code = (requestMessage.request_api_version < 0 || requestMessage.request_api_version > 4) ? htons(UNSUPPORTED_VERSION) : htons(0);
        responseMessage.response_body = responseBody;

        // Write back to client
        socket.writeBufferToClient(client_fd, responseMessage);
    }
    if(client_fd != -1)
        close(client_fd);
    return 0;
}