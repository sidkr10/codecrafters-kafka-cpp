#ifndef SOCKET_H
#define SOCKET_H

#pragma once

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>

class Socket
{
public:

    Socket();
    ~Socket();

    int createSocket();
    int acceptConnection();

    uint8_t* readBufferFromClient(int client_fd);
    void writeBufferToClient(int client_fd, const uint8_t *buffer, size_t &buffer_size);

private:
    int server_fd;
};

#endif