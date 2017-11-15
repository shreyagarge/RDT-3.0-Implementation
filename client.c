//RDT 3.0 WITH PIPELINING AND SELECTIVE REPEAT WITH CUMULATIVE ACKNOWLEDGEMENT SENDER

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
#include <pthread.h>
#include <stdbool.h>

struct Packet{
	int seqNo;
	int ackNo;
	char payload[10];
};

int sockfd, sendWindow=4, sendBase=0, i=0, j=0, k=0, l=0, fileSize=0, fdMax, returnVal, check=-1, seqNoOfPacketToDrop=-1, option, orderFlag=0, seqNoOfPacketToUnorder;
bool ackRecieved[205];
struct Packet packet[205], ackPacket;
struct addrinfo hints, *ai;
pthread_t thread;
pthread_attr_t attributes;

void *AckReciever(void *packetPointer)
{
	struct Packet *currentPacket;
	currentPacket = (struct Packet *) packetPointer;

	fd_set readFds;
	FD_ZERO(&readFds);
	FD_SET(sockfd, &readFds);
	fdMax = sockfd;

	int flag;
	WaitingForAck :
		flag=0;
		while(flag == 0){
			struct timeval tv;
			if (seqNoOfPacketToDrop == currentPacket->seqNo){
				tv.tv_sec = 0;
	            tv.tv_usec = 5;			
			}
			else{
				tv.tv_sec = 1;
	            tv.tv_usec = 0;	
			}			
			returnVal = select(fdMax+1, &readFds, NULL, NULL, &tv); 
			if (returnVal == -1) 
			{
    		  	printf("Error in select() !\n");
    		   	exit(1);
			}
			else 
			if (returnVal == 0 && flag == 0 && ackRecieved[currentPacket->seqNo] == 0) 
			{
				printf("**Timeout occurred for Packet with SEQNO = %d ! Retransmitting...**\n\n", currentPacket->seqNo);
				if(send(sockfd, currentPacket, sizeof(struct Packet), 0) == -1)
				{
					printf("Error in retransmitting Packet !\n");
					exit(1);
				}
				seqNoOfPacketToDrop=-1;
				goto WaitingForAck;
			}
			else
			{
				if (recv(sockfd, &ackPacket, sizeof(ackPacket), 0) == -1)
				{
					printf("Error in recieving Ack \n");
					exit(1);
				}
				flag=1;
				if(sendBase <= ackPacket.ackNo)
				{
					int m;
					m=ackPacket.ackNo-1;
					while(m >= sendBase)
					{
						ackRecieved[m]=true;
						m--;
					}
					sendBase=ackPacket.ackNo;
				}
				else
					ackRecieved[ackPacket.ackNo-1]=true;
				pthread_exit((void *) NULL);
			}
		}
}


int main(int argc, char** argv) 
{
	
	char serverPortNo[10];
	strcpy(serverPortNo, argv[1]);

	if(pthread_attr_init(&attributes) != 0) {
 		printf("Error initalizing thread attributes !\n");
 		exit(1);
	}    
	if(pthread_attr_setdetachstate(&attributes,PTHREAD_CREATE_DETACHED) != 0) {
 		printf("Error detaching thread !\n");
 		exit(1);
	}   

	for(i=0;i<205;i++)
		ackRecieved[i]=false;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 

	if(getaddrinfo(NULL, serverPortNo, &hints, &ai) != 0) 
	{
		printf("Error on Getaddrinfo() !\n");
		exit(1);
	}
	if((sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) 
	{
		printf("Error on Socket creation !\n");
		exit(1);
	}	
	if(connect(sockfd, ai->ai_addr, ai->ai_addrlen) == -1) 
	{
		printf("Error on Socket connection !\n");
		exit(1);	
	}

	FILE *fp = fopen("bigfile.txt","r");
	if (fp == NULL){
		printf("Could not open file.txt !");
		exit(1);
	}
	char string[10];
	while(fgets(string, 10, fp)){
		packet[j].seqNo=j;
		packet[j].ackNo=k;
		strcpy(packet[j].payload,string);
		j++;
	}
	fileSize=j;

	printf("\n1. Simulate packet sending with no loss.\n");
	printf("2. Simulate packet loss.\n");
	printf("3. Simulate out of order packets.\n");
	printf("\nEnter an option (1 or 2 or 3) : ");
	scanf("%d", &option);

	if(option == 1)
		goto sendPacket;
	else if (option == 2){	
		printf("Enter the sequence number of a packet to drop : ");
		scanf("%d", &seqNoOfPacketToDrop);
		goto sendPacket;
	}
	else if (option == 3){
		printf("Enter the sequence number of a packet to sent out of order : ");
		scanf("%d", &seqNoOfPacketToUnorder);
		goto outOfOrder;
	}
	else{
		printf("Wrong option entered !\n");
		exit(1);
	}
	
	sendPacket:
		while(packet[l].seqNo < fileSize-1) {		       
			if (packet[l].seqNo < (sendBase + sendWindow) && packet[l].seqNo != check) {	
				if(send(sockfd, &packet[l], sizeof(packet[l]), 0) == -1)
				{
					printf("Error in sending Packet with SEQNO = %d !\n", packet[l].seqNo);
					exit(1);
				}
			
				if (pthread_create(&thread, &attributes, &AckReciever, (void *)&packet[l]) != 0) {
			        printf("Error in thread creation for packet with SEQNO = %d !\n", packet[l].seqNo);
					exit(1);
			    }	

				check=packet[l].seqNo;
				packet[l].seqNo++;
				packet[l].ackNo=k;
				l++;
	
				sleep(0.999);
			}
		}

		if(send(sockfd, &packet[fileSize-1], sizeof(packet[fileSize-1]), 0) == -1)
		{
			printf("Error in sending Packet with SEQNO = %d !\n", packet[fileSize-1].seqNo);
			exit(1);
		}
			
		if (pthread_create(&thread, &attributes, &AckReciever, (void *)&packet[fileSize-1]) != 0) {
	        printf("Error in thread creation for packet with SEQNO = %d !\n", packet[fileSize-1].seqNo);
			exit(1);
	    }

	goto end;

	outOfOrder:
		while(packet[l].seqNo < seqNoOfPacketToUnorder) {		       
			if (packet[l].seqNo < (sendBase + sendWindow) && packet[l].seqNo != check) {	
				if(send(sockfd, &packet[l], sizeof(packet[l]), 0) == -1)
				{
					printf("Error in sending Packet with SEQNO = %d !\n", packet[l].seqNo);
					exit(1);
				}
			
				if (pthread_create(&thread, &attributes, &AckReciever, (void *)&packet[l]) != 0) {
			        printf("Error in thread creation for packet with SEQNO = %d !\n", packet[l].seqNo);
					exit(1);
			    }	

				check=packet[l].seqNo;
				packet[l].seqNo++;
				packet[l].ackNo=k;
				l++;
	
				sleep(0.999);
			}
		}

		check=packet[l].seqNo;
		packet[l].seqNo++;
		packet[l].ackNo=k;
		l++;
	
		sleep(0.999);

		while(packet[l].seqNo < seqNoOfPacketToUnorder+3) {		       
			if (packet[l].seqNo < (sendBase + sendWindow) && packet[l].seqNo != check) {	
				if(send(sockfd, &packet[l], sizeof(packet[l]), 0) == -1)
				{
					printf("Error in sending Packet with SEQNO = %d !\n", packet[l].seqNo);
					exit(1);
				}
			
				if (pthread_create(&thread, &attributes, &AckReciever, (void *)&packet[l]) != 0) {
			        printf("Error in thread creation for packet with SEQNO = %d !\n", packet[l].seqNo);
					exit(1);
			    }	

				check=packet[l].seqNo;
				packet[l].seqNo++;
				packet[l].ackNo=k;
				l++;
	
				sleep(0.999);
			}
		}

		if(send(sockfd, &packet[seqNoOfPacketToUnorder-1], sizeof(packet[seqNoOfPacketToUnorder-1]), 0) == -1)
		{
			printf("Error in sending Packet with SEQNO = %d !\n", packet[seqNoOfPacketToUnorder-1].seqNo);
			exit(1);
		}
	
		if (pthread_create(&thread, &attributes, &AckReciever, (void *)&packet[4]) != 0) {
	        printf("Error in thread creation for packet with SEQNO = %d !\n", packet[4].seqNo);
			exit(1);
	    }

		while(packet[l].seqNo < fileSize-1) {		       
			if (packet[l].seqNo < (sendBase + sendWindow) && packet[l].seqNo != check) {	
				if(send(sockfd, &packet[l], sizeof(packet[l]), 0) == -1)
				{
					printf("Error in sending Packet with SEQNO = %d !\n", packet[l].seqNo);
					exit(1);
				}
			
				if (pthread_create(&thread, &attributes, &AckReciever, (void *)&packet[l]) != 0) {
			        printf("Error in thread creation for packet with SEQNO = %d !\n", packet[l].seqNo);
					exit(1);
			    }	

				check=packet[l].seqNo;
				packet[l].seqNo++;
				packet[l].ackNo=k;
				l++;
	
				sleep(0.999);
			}
		}

		if(send(sockfd, &packet[fileSize-1], sizeof(packet[fileSize-1]), 0) == -1)
		{
			printf("Error in sending Packet with SEQNO = %d !\n", packet[fileSize-1].seqNo);
			exit(1);
		}
			
		if (pthread_create(&thread, &attributes, &AckReciever, (void *)&packet[fileSize-1]) != 0) {
	        printf("Error in thread creation for packet with SEQNO = %d !\n", packet[fileSize-1].seqNo);
			exit(1);
	    }	

	end:
		check=-1;

}

