#include<bits/stdc++.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/ip.h> /* superset of previous */
#include <netinet/tcp.h>

using namespace std;

int main()
{
    int sock_r = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if(sock_r == 1) {
        perror("Socket create korte pare nai\n");
        return -1;
    }

    unsigned char *buf = new unsigned char[65535];
    memset(buf, 0, sizeof(buf));

    sockaddr saddr;
    int saddr_len = sizeof(saddr);

    int buflen=recvfrom(sock_r, buf, 65535, 0, &saddr, (socklen_t *)&saddr_len);
    printf("Received bytes = %d\n", buflen);
    if(buflen == -1)
    {
        perror("receive korte pare nai\n");
        return -1;
    }

    cout << "Packet:\n" << endl;
    ethhdr *eth = (ethhdr *)(buf);
    printf("\nEthernet Header\n");
    printf("\t|-Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
    printf("\t|-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
    printf("\t|-Protocol : %d\n",eth->h_proto);

    cout << endl << endl;

    sockaddr_in source, dest;

    iphdr *ip = (iphdr*)(buf + sizeof(ethhdr));
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = ip->saddr;
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = ip->daddr;
    unsigned short iphdrlen = (unsigned int)ip->ihl * 4;
    
    printf("\t|-Version : %d\n",(unsigned int)ip->version);
    printf("\t|-Internet Header Length : %d DWORDS or %d Bytes\n",(unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4);
    printf("\t|-Type Of Service : %d\n",(unsigned int)ip->tos);
    printf("\t|-Total Length : %d Bytes\n",ntohs(ip->tot_len));
    printf("\t|-Identification : %d\n",ntohs(ip->id));
    printf("\t|-Time To Live : %d\n",(unsigned int)ip->ttl);
    printf("\t|-Protocol : %d\n",(unsigned int)ip->protocol);
    printf("\t|-Header Checksum : %d\n",ntohs(ip->check));
    printf("\t|-Source IP : %s\n", inet_ntoa(source.sin_addr));
    printf("\t|-Destination IP : %s\n",inet_ntoa(dest.sin_addr));

    printf("\nTCP header\n");
    unsigned short tcphdrlen;
    tcphdr *tcp = (tcphdr*)(buf + sizeof(ethhdr) + iphdrlen);

    printf("\t|-Source Port: %d\n", ntohs(tcp->source));

    unsigned char * data = (buf + iphdrlen + sizeof(eth) + sizeof(tcp));

    int remaining_data = buflen - (iphdrlen + sizeof(eth) + sizeof(tcp));
    printf("Data size = %d\n", remaining_data);
 
    for(int i=0;i<remaining_data;i++)
    {
        if(i!=0 && i%16==0) printf("\n");
        printf(" %.2X ",data[i]);
    }
    cout << endl << endl;

    delete[] buf;

    return 0;
}