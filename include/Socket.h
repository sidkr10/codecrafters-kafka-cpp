#ifndef SOCKET_H
#define SOCKET_H

#pragma once

#include <netdb.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <sys/types.h>

class Socket
{
public:
    struct RequestMessage {
        u_int32_t message_size;
        u_int16_t request_api_key;
        u_int16_t request_api_version;
        u_int32_t correlation_id;

        void fromBuffer(const u_int8_t *buffer, size_t buffer_size);
    };
    struct ResponseMessage {
        u_int32_t message_size;
        // u_int16_t request_api_key;
        u_int16_t request_api_version;
        u_int32_t correlation_id;
        
        u_int8_t* toBuffer();
    };
    Socket();
    ~Socket();

    int createSocket();
    int acceptConnection();

    RequestMessage readBufferFromClient(int client_fd);
    void writeBufferToClient(int client_fd, ResponseMessage &responseMessage);

private:
    int server_fd;
};

#endif