#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>

int main()
{
    sockaddr_in server;
    sockaddr_in client;

    int clientlen;
    char buf[1500];

    // step 1
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // step 2
    memset((char*) &server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(9090);

    if(bind(sock, (sockaddr*) &server, sizeof(server)) < 0)
        perror("ERROR on binding");

    // step 3
    while(true)
    {
        bzero(buf, 1500);
        recvfrom(sock, buf, 1500-1, 0, (sockaddr*) &client, (socklen_t*) &clientlen);
        printf("%s\n", buf);
    }

    close(sock);
    
    return 0;
}