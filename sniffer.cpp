#include <pcap.h>
#include <stdio.h>
#include <arpa/inet.h>
//#include <netinet/ip.h>
#include <netinet/tcp.h>
#include "http_parser.hpp"
using namespace std;

/* Ethernet header */
struct ethheader {
  u_char  ether_dhost[6]; /* destination host address */
  u_char  ether_shost[6]; /* source host address */
  u_short ether_type;     /* protocol type (IP, ARP, RARP, etc) */
};

/* IP Header */
struct ipheader {
  unsigned char      iph_ihl:4, //IP header length
                     iph_ver:4; //IP version
  unsigned char      iph_tos; //Type of service
  unsigned short int iph_len; //IP Packet length (data + header)
  unsigned short int iph_ident; //Identification
  unsigned short int iph_flag:3, //Fragmentation flags
                     iph_offset:13; //Flags offset
  unsigned char      iph_ttl; //Time to Live
  unsigned char      iph_protocol; //Protocol type
  unsigned short int iph_chksum; //IP datagram checksum
  struct  in_addr    iph_sourceip; //Source IP address
  struct  in_addr    iph_destip;   //Destination IP address
};

struct tcpheader {
    unsigned short int source; // Source port
    unsigned short int dest; // Destination port
    unsigned int seq; // Sequence number
    unsigned int ack_seq; // Acknowledgement Number
    unsigned short int   res1:4,
            doff:4, // Header length
            fin:1,
            syn:1,
            rst:1,
            psh:1,
            ack:1,
            urg:1,
            ece:1,
            cwr:1;
    unsigned short int window; // Window size
    unsigned short int check; // checksum
    unsigned short int urg_ptr; // Urgent pointer
};

void got_packet(u_char *args, const struct pcap_pkthdr *header,
                              const u_char *packet)
{
  struct ethheader *eth = (struct ethheader *)packet;

  if (ntohs(eth->ether_type) == 0x0800) { // 0x0800 is IP type
    struct ipheader * ip = (struct ipheader *)
                           (packet + sizeof(struct ethheader));

    if(ip->iph_protocol != IPPROTO_TCP) return;
    

    struct tcphdr *tcphh = (struct tcphdr*) (packet + sizeof(struct ethheader) + ip->iph_ihl * 4);
    int data_size = ntohs(ip->iph_len) - ip->iph_ihl * 4 - tcphh->doff * 4;
    
    char* data = (char*) (packet + sizeof(ethheader) + ip->iph_ihl * 4 + tcphh->doff * 4);

    http_parser parser(data);
    if(parser.isHTTP() == false) return;

    printf("Total data size: %hu\n", data_size);
    printf("           From: %s\n", inet_ntoa(ip->iph_sourceip));  
    printf("             To: %s\n\n", inet_ntoa(ip->iph_destip));


    /* determine protocol */
    switch(ip->iph_protocol) {                                 
        case IPPROTO_TCP:
            printf("HTTP Message:\n");
            for(int i = 0; i < data_size; i++) printf("%c", data[i]);
            printf("\n\n");

            return;
        case IPPROTO_UDP:
            //printf("   Protocol: UDP\n");
            return;
        case IPPROTO_ICMP:
            //printf("   Protocol: ICMP\n");
            return;
        default:
            //printf("   Protocol: others\n");
            return;
    }
  }
}

int main()
{
  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];
  struct bpf_program fp;
  char filter_exp[] = "ip proto \\tcp"; // capturing only IPv4 packets with TCP protocol
  bpf_u_int32 net;

  // Step 1: Open live pcap session on NIC with name enp0s3
  handle = pcap_open_live("enp0s3", BUFSIZ, 1, 1000, errbuf);
  if(handle == NULL)
  {
    printf("Failed to create socket\n");
    printf("Error message: %s\n", errbuf);
    return 0;
  }

  // Step 2: Compile filter_exp into BPF psuedo-code
  char pre[] = "Error ";
  int compile_res = pcap_compile(handle, &fp, filter_exp, 0, net);
  if(compile_res == PCAP_ERROR) {
    printf("Failed to compile BPF\n");
    pcap_perror(handle, pre);
    return 0;
  }

  int setup_res = pcap_setfilter(handle, &fp);
  if(setup_res == PCAP_ERROR) {
    printf("Failed to attach BPF\n");
    pcap_perror(handle, pre);
    return 0;
  }

  // Step 3: Capture packets
  pcap_loop(handle, -1, got_packet, NULL);

  pcap_close(handle);   //Close the handle

  return 0;
}