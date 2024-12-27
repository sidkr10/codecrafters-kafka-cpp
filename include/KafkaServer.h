#ifndef KAFKASERVER_H
#define KAFKASERVER_H

#pragma once

#include <Socket.h>

class KafkaServer
{
public:
    KafkaServer();
    ~KafkaServer();
    int acceptConnections();
    int handleClient(int client_fd);
private:
    Socket socket;
    int server_fd;
};

#endif