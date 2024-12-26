#ifndef SOCKET_H
#define SOCKET_H

#pragma once

#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <sys/types.h>
#include <vector>

#define UNSUPPORTED_VERSION 35
#define MIN_SUPPORTED_API_VERSION 4
#define MAX_SUPPORTED_API_VERSION 18

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

    // API Version Entry
    struct ApiVersion {
        uint16_t api_key;
        uint16_t min_supported_version = htons(MIN_SUPPORTED_API_VERSION);
        uint16_t max_supported_version = htons(MAX_SUPPORTED_API_VERSION);
        uint8_t tag_buffer = 0;

        // Serialize to buffer
        void toBuffer(std::vector<uint8_t>& buffer) const;
    };

    // API Versions Response Body
    struct ApiVersionsResponseBody {
        uint16_t error_code;
        uint8_t api_arr_len;
        std::vector<ApiVersion> api_versions;
        uint32_t throttle_time = htonl(0);
        uint8_t tag_buffer = 0;

        // Serialize to buffer
        void toBuffer(std::vector<uint8_t>& buffer) const;
    };

    struct ResponseMessage {
        u_int32_t message_size;
        u_int32_t correlation_id;
        ApiVersionsResponseBody response_body;

        void toBuffer(std::vector<u_int8_t>& buffer) const;
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