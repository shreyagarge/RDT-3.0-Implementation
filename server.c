//RDT 3.0 WITH PIPELINING AND SELECTIVE REPEAT WITH CUMULATIVE ACKNOWLEDGEMENT RECIEVER

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

	
char server_port_number[10];
int sockfd, newfd, rv, fd_max, i, j, k, m=0, nbytes, rcv_wnd=4, rcv_base=0, flag_end=0;
struct addrinfo hints, *servinfo, *p;
fd_set master, read_fds;
char clientIP[INET6_ADDRSTRLEN], buffer[1024], file_buff[500][50];
socklen_t addrlen;
struct sockaddr_storage client;

struct Packet{
	int seqNo;
	int ackNo;
	char payload[10];
};

struct Packet packet, ack_packet;

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) 
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv) 
{
	strcpy(server_port_number, argv[1]);

	for(k=0;k<500;k++)
		strcpy(file_buff[k],"");

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 

	if((rv = getaddrinfo(NULL, server_port_number, &hints, &servinfo)) != 0){
		printf("Getaddrinfo() failed !\n");
		exit(1);
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			continue;
		}
		break;
	}

	if(p == NULL){
		printf("Failed to bind server socket !\n");
		exit(1);
	}

	freeaddrinfo(servinfo);

	if(listen(sockfd, 10) == -1)
	{
		printf("Listening error in server socket !\n");
		exit(1);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&master);
	FD_SET(sockfd, &master);
	fd_max = sockfd;

	printf("Waiting for sender....\n");

	while(1) 
	{
       	read_fds = master;
      	if (select(fd_max+1, &read_fds, NULL, NULL, NULL) == -1) 
        {
      		printf("Select error !\n");
       		exit(1);
       	}

		for(i = 0; i <= fd_max; i++) 
        {
			if (FD_ISSET(i, &read_fds)) 
	        {
		       	int hash_socket;
				if (i == sockfd) 
				{
					addrlen = sizeof client;
					newfd = accept(sockfd,(struct sockaddr *)&client,&addrlen);

					if (newfd == -1) 
					{
						printf("Accept error !\n");
						exit(1);
					} 
					else 
					{
                       	FD_SET(newfd, &master);
                       	if (newfd > fd_max) 
                       	{ 
                       	    fd_max = newfd;
                       	}
                   
				     	hash_socket = newfd;
                        printf("New connection from %s on socket %d !\n", inet_ntop(client.ss_family, get_in_addr((struct sockaddr*)&client), clientIP, INET6_ADDRSTRLEN), newfd);
                   	}
               	}
               	else 
               	{
					if ((nbytes = recv(i, &packet, sizeof(packet), 0)) <= 0) 
					{
						if (nbytes == 0) {
							printf("Socket %d hung up !\n", i);
							exit(0);
						} 
						else {
							printf("Packet Recieve error on socket %d !\n", i);
							exit(1);
						}
						close(i);
						FD_CLR(i, &master);
					} 
					else 
					{
						printf("Recieved packet with SEQNO = %d and ACKNO = %d\n", packet.seqNo, packet.ackNo);
 						
						if(packet.seqNo > (rcv_base - rcv_wnd) && packet.seqNo < (rcv_base + rcv_wnd - 1)){

							if(strcmp(file_buff[packet.seqNo],"") != 0){
								printf("Possible duplicate packet - SEQNO = %d !\n\n", packet.seqNo);
							}
							else
								printf("\n");
							
							strcpy(file_buff[packet.seqNo], packet.payload);

												
							if(packet.seqNo == rcv_base){
								int l;
								l=rcv_base;
								FILE *fp=fopen("newFile.txt","a");
								if (fp == NULL){
									printf("Could not open File to read from !");
									exit(1);
								}
								while(strcmp(file_buff[l],"") != 0){
									fputs(file_buff[l],fp);
									l++;
								}
								fclose(fp);
								rcv_base=l;

								ack_packet.seqNo = m;
								ack_packet.ackNo = rcv_base;
								strcpy(ack_packet.payload,"");
								if ((nbytes = send(i, &ack_packet, sizeof(ack_packet), 0)) == -1){
									printf("Ack Send error on socket %d !\n", i);
									exit(1);		
								}
							}
							else
							{
								ack_packet.seqNo = m;
								ack_packet.ackNo = rcv_base;
								if ((nbytes = send(i, &ack_packet, sizeof(ack_packet), 0)) == -1){
									printf("Ack Send error on socket %d !\n", i);
									exit(1);		
								}
							}
						}	//	close checking condition
					}//	close packet recieved properly
				}//	close packet recieved
			}//	close isset
		}//	close for
    }//	close while

}
