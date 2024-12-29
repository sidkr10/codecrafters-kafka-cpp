#ifndef KAFKASERVER_H
#define KAFKASERVER_H

#pragma once

#include <Socket.h>
#include <KafkaMessage.h>
#include <ErrorCode.h>

class KafkaServer
{
public:
    KafkaServer();
    ~KafkaServer();
    int acceptConnections();
    int handleClient(int client_fd);
    size_t processApiVersionsReqeuest(const uint8_t* request, uint8_t* response);
    size_t processDescribeTopicPartitionsRequest(const uint8_t* request, uint8_t* response);
private:
    Socket socket;
    int server_fd;
};

#endif