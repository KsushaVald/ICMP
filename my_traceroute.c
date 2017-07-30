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
#include <sys/poll.h>

struct packet{
        struct icmphdr hdr;
        char message[64-sizeof(struct icmphdr)];
};

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

int main(){
	struct hostent *host_name, *h_name; struct iphdr *ip; struct icmphdr *icmp;
        struct sockaddr_in s_addr, r_addr;
	socklen_t len=sizeof(struct sockaddr);
        char str_name[16];
	int ttl=0; int fd_socket; int cnt=0; int i, test;
	struct packet  pack; struct in_addr help;
 	unsigned char buf[sizeof(struct packet)+sizeof(struct iphdr)];
	pid_t pid=getpid(); struct pollfd p;

	memset(str_name,0,16);
        printf("Enter the host name: ");
        scanf("%s",str_name);
	host_name=gethostbyname(str_name);
        if(host_name==NULL){
		perror("gethostbyname");
		exit(-1);
	}
        bzero(&s_addr,sizeof(s_addr));
        s_addr.sin_family=host_name->h_addrtype;
        s_addr.sin_port=0;
        s_addr.sin_addr.s_addr=*(long*)host_name->h_addr;
	fd_socket=socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	bzero(&pack,sizeof(pack));
	pack.hdr.type=ICMP_ECHO;
	pack.hdr.un.echo.id=pid;
	for(i=0; i<sizeof(pack.message)-1; i++)
		pack.message[i]=i+'0';
	pack.message[i]=0;
	pack.hdr.un.echo.sequence=cnt++;
	pack.hdr.checksum=checksum(&pack,sizeof(pack));
	p.fd=fd_socket;
	p.events=POLLIN;
	do{
		socklen_t len=sizeof(struct sockaddr);
		ttl++;
		memset(buf,0,sizeof(buf));
		if(setsockopt(fd_socket,SOL_IP,IP_TTL,&ttl,sizeof(ttl))==-1){
			perror("setcokopt");
			exit(-1);
		}
		if(sendto(fd_socket,&pack,sizeof(pack),0,(struct sockaddr*)&s_addr,sizeof(s_addr))==-1){
			perror("sendto");
			exit(-1);
		}
		test=poll(&p,1,2000);
		if(test==-1){
			perror("poll");
			exit(-1);
		}
		else{
			if(test=!0){
				if(p.revents&POLLIN){
					if(recvfrom(fd_socket,&buf,sizeof(buf),0,(struct sockaddr*)&r_addr,&len)>0){
						ip=(struct iphdr*)buf;
						icmp=(struct icmphdr*)(buf+sizeof(struct icmphdr));
						help.s_addr=ip->saddr;
						printf("%d: %s\n",ttl,inet_ntoa(help));
					}
					else{
						perror("recvfrom");
						exit(-1);
					}
				}
				else
					printf("%d: ***\n", ttl);
			}
		}
	}while(r_addr.sin_addr.s_addr!=s_addr.sin_addr.s_addr);
}
