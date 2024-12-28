#include <csignal>
#include <KafkaServer.h>

int main(int argc, char *argv[])
{
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    signal(SIGCHLD, SIG_IGN);
    KafkaServer kafka;
    
    while(true) {
        int client_fd = kafka.acceptConnections();
        // Create a child process
        int pid = fork();

        // Child process shall handle the clients and parent process will accept new connections
        if(pid == 0) {
            int res = kafka.handleClient(client_fd);
            if(res == -1)
                close(client_fd);
            _exit(0);
        } else {
            // Exit if no client connections received
            if (client_fd < 0)
                break;
        }
    }
    return 0;
}