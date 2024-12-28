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
        Socket::RequestMessage requestMessage = socket.readBufferFromClient(client_fd);

        if (requestMessage.message_size == 0)
            break;

        // Create a response message with request correlation id
        Socket::ResponseMessage responseMessage;
        responseMessage.correlation_id = htonl(requestMessage.correlation_id);

        Socket::ApiVersionsResponseBody responseBody;

        Socket::ApiVersion api_version1;
        api_version1.api_key = htons(18);
        Socket::ApiVersion api_version2;
        api_version2.api_key = htons(75);
        api_version2.min_supported_version = htons(0);
        api_version2.max_supported_version = htons(4);

        responseBody.api_versions.push_back(api_version1);
        responseBody.api_versions.push_back(api_version2);

        responseBody.error_code = (requestMessage.request_api_version < 0 || requestMessage.request_api_version > 4) ? htons(UNSUPPORTED_VERSION) : htons(0);
        responseMessage.response_body = responseBody;

        // Write back to client
        socket.writeBufferToClient(client_fd, responseMessage);
    }
    return 1;
}

