#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <fcntl.h>

struct my_icmp{
	u_char type;
	u_char code;
	u_short chsum;
	u_short id;
	u_short seq;
};

int main(){
	struct sockaddr_in s_addr;
	int fd_socket;
	u_char datagram[1500]; u_char packet_r[1500]; u_char *packet_s;
	struct iphdr *ip; struct my_icmp *icmp, *icmp_r;
	socklen_t len=sizeof(struct sockaddr);
	int flag=1; int cou=0; int bytes,test;

	memset(datagram,0,1500);
	memset(packet_r,0,1500);

	s_addr.sin_family=AF_INET;
	s_addr.sin_port=htons(37000);
	s_addr.sin_addr.s_addr=inet_addr("8.8.8.8");
	fd_socket=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
	if(fd_socket<0){
		perror("socket");
		exit(-1);
	}
	if(setsockopt(fd_socket,IPPROTO_IP,IP_HDRINCL,&flag,sizeof(flag))==-1){
		perror("setsockopt");
		exit(-1);
	}
	fcntl(fd_socket,F_SETFL,O_NONBLOCK);
/*	if(bind(fd_socket,(struct sockaddr*)&s_addr,sizeof(s_addr))==-1){
		perror("bind");
		exit(-1);
	}*/
	ip=(struct iphdr*)datagram;
	ip->version=4;
	ip->ihl=5;
	ip->tos=0;
	ip->tot_len=htons(sizeof(struct iphdr)+sizeof(struct my_icmp)+10);
	ip->ttl=255;
	ip->protocol=IPPROTO_ICMP;
	ip->check=0;
	ip->saddr=inet_addr("192.168.2.1");
	ip->daddr=inet_addr("8.8.8.8");
	icmp=(struct my_icmp*)(datagram+sizeof(struct iphdr));
	icmp->type=htons(ICMP_ECHO);
	icmp->code=0;
	icmp->chsum=0;
	icmp->id=htons(getpid());
	icmp->seq=htons(cou++);
	packet_s=malloc(sizeof(struct iphdr)+sizeof(struct my_icmp)+10);
	memcpy(packet_s,ip,sizeof(struct iphdr));
	memcpy(packet_s+sizeof(struct iphdr),icmp,sizeof(struct my_icmp));
	if(connect(fd_socket,(struct sockaddr*)&s_addr,sizeof(s_addr))==-1){
		perror("connect");
		exit(-1);
	}
	send(fd_socket,packet_s, sizeof(packet_s),0);
/*	if(sendto(fd_socket,&packet_s,sizeof(packet_s),0,(struct sockaddr*)&s_addr,sizeof(s_addr))==-1){
		perror("sendto");
		exit(-1);
	}
	if(recvfrom(fd_socket,&packet_r,sizeof(packet_r),0,(struct sockaddr*)&s_addr, &len)==-1){
		perror("recvfrom");
		exit(-1);
	}*/
	recv(fd_socket,packet_r,sizeof(packet_r),0);
	icmp_r=(struct my_icmp*)(packet_r+sizeof(struct iphdr));
	printf("icmp_type %d\n",icmp_r->type);
	printf("icmp_sequence %d\n",icmp_r->seq);
	printf("icmp_id %d\n",icmp_r->id);
	printf("cheacksumm %d\n",icmp_r->chsum);
}
