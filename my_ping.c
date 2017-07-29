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
#include <resolv.h>

struct packet{
	struct icmphdr hdr;
	char message[64-sizeof(struct icmphdr)];
};


pid_t pid=-1;

void unpacking(unsigned char *buf){
	struct iphdr *ip;
	struct icmphdr *icmp;
	ip=(struct iphdr*)buf;
	icmp=(struct icmphdr*)(buf+sizeof(struct iphdr));
	if(icmp->un.echo.id==pid){
		printf("ICMP: type %d, code %d, checksum %d, id %d, seq %d\n", icmp->type, icmp->code, ntohs(icmp->checksum), icmp->un.echo.id, icmp->un.echo.sequence);
	}
}

unsigned short checksum(void *buf, unsigned int size){
	unsigned short *pack_buf=buf;
	unsigned short result;
	unsigned int sum=0;
	while(size>1){
		sum+=*pack_buf++;
		size-=2;
	}
	if(size>0){
		sum+=((*pack_buf)&htons(0xFF00));
	}
	while(sum>>16){
		sum=(sum & 0xffff)+(sum>>16);
	}
	result=~sum;
	return(result);
}

void sending_module(){
	unsigned char buf[sizeof(struct packet)+sizeof(struct iphdr)];
	int fd_socket;
	struct sockaddr_in s_addr;
	fd_socket=socket(PF_INET,SOCK_RAW,IPPROTO_ICMP);
	if(fd_socket==-1){
		perror("socket");
		exit(-1);
	}
	while(1){
		int test; socklen_t len=sizeof(struct sockaddr);
		memset(buf, 0, sizeof(buf));
		test=recvfrom(fd_socket,buf,sizeof(buf),0,(struct sockaddr*)&s_addr,&len);
		if(test>0)
			unpacking(buf);
		else{
			perror("recvfrom");
			exit(-1);
		}
	}
}

void receiving_module(struct sockaddr_in *s_addr){
	struct packet pack; int fd_socket;
	struct sockaddr_in r_addr;
	int cnt=1, flag=255;
	fd_socket=socket(PF_INET,SOCK_RAW,IPPROTO_ICMP);
	if(fd_socket<0){
		perror("socket");
		exit(-1);
	}
	if(setsockopt(fd_socket,SOL_IP, IP_TTL,&flag,sizeof(flag))==-1){
		perror("setsockopt");
		exit(-1);
	}
	if(fcntl(fd_socket,F_SETFL,O_NONBLOCK)==-1){
		perror("fcntl");
		exit(-1);
	}
	while(1){
		socklen_t len=sizeof(struct sockaddr); int i;
		printf("Message %d\n",cnt);
		if(recvfrom(fd_socket,&pack,sizeof(pack),0,(struct sockaddr*)&r_addr,&len))
			printf("***Got message!***\n");
		bzero(&pack,sizeof(pack));
		pack.hdr.type=ICMP_ECHO;
		pack.hdr.un.echo.id=pid;
		for(i=0; i<sizeof(pack.message)-1; i++)
			pack.message[i]=i+'0';
		pack.message[i]=0;
		pack.hdr.un.echo.sequence=cnt++;
		pack.hdr.checksum=checksum(&pack,sizeof(pack));
		if(sendto(fd_socket,&pack,sizeof(pack),0,(struct sockaddr*)s_addr,sizeof(*s_addr))==-1){
			perror("sendto");
			exit(-1);
		}
		sleep(1);
	}
}

int main(){
	struct hostent *host_name;
	struct sockaddr_in s_addr;
	char str_name[16];
	pid_t pid_ch; 

	pid=getpid();
	memset(str_name,0,16);
	printf("Enter the host name: ");
	scanf("%s",str_name);
	host_name=gethostbyname(str_name);
	if(host_name!=NULL){
		bzero(&s_addr,sizeof(s_addr));
		s_addr.sin_family=host_name->h_addrtype;
		s_addr.sin_port=0;
		s_addr.sin_addr.s_addr=*(long*)host_name->h_addr;

		if(fork()==0)
			sending_module();
		else
			receiving_module(&s_addr);
	}
	else
		perror("gethostbyname");
}

