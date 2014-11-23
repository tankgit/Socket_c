#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <x86_64-linux-gnu/sys/socket.h>
#include <x86_64-linux-gnu/sys/types.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

//delet the \n at the end of string
char *delN(char *a)
{
	int l=strlen(a);
	a[l-1]=0;
	return a;
}

int main(int argc,char *argv[])
{
	struct sockaddr_in clientaddr;
	pid_t pid;
	int clientfd,sendbytes;
	struct hostent *host;
	char *buf,*buf_r,name[50];
	if(argc < 3)
	{
		printf("usage:\n");
		printf("\033[31musage:\n%s host port\n\033[0m",argv[0]);
		exit(1);
	}
	host = gethostbyname(argv[1]);
	if((clientfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("socket\n");
		exit(1);
	}

	//------bind client socket------

	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons((uint16_t)atoi(argv[2]));
	clientaddr.sin_addr = *((struct in_addr*)host->h_addr);
	bzero(&(clientaddr.sin_zero),0);

	//------------------------------

	//------connect to service------

	if(connect(clientfd,(struct sockaddr*)&clientaddr,sizeof(struct sockaddr)) == -1)
	{
		perror("connect\n");
		exit(1);
	}
	buf = (char*)malloc(120);
	memset(buf,0,120);
	buf_r = (char*)malloc(100);
	if(recv(clientfd,buf,100,0) == -1)
	{
		perror("recv");
		exit(1);
	}
	printf("%s\n",buf);

	//------------------------------

	//------Login and Authenticate------

	printf("\033[33mplease input the username:\033[0m\n");
	memset(buf,0,255);
	fgets(buf,120,stdin);
	strcpy(name,delN(buf));
	if(send(clientfd,name,strlen(name),0)==-1)
	{
		perror("send");
		exit(1);
	}
	printf("\033[33mplease input the password:\033[0m\n");
	memset(buf,0,255);
	fgets(buf,120,stdin);
	if(send(clientfd,delN(buf),strlen(buf),0)==-1)
	{
		perror("send");
		exit(1);
	}
	memset(buf,0,255);
	if(recv(clientfd,buf,255,0)==-1)
	{
		perror("recv");
		exit(1);
	}
	if(!strcmp(buf,"FAILED"))
	{
		printf("\033[31mIncorrect username or password!\n\033[0m");
		exit(1);
	}
	printf("\033[32mLogin successfully!\n\033[0m");

	//----------------------------------

	//------start conversation------

	pid = fork(); //create the child process
	while(1)
	{
		if(pid>0)
		{
		//parent process: send msg
			strcpy(buf,name);
			strcat(buf,":");
			memset(buf_r,0,100);
			fgets(buf_r,100,stdin);
			strncat(buf,buf_r,strlen(buf_r)-1);
			if((sendbytes = send(clientfd,buf,strlen(buf),0)) == -1)
			{
				perror("send\n");
				exit(1);
			}
		}
		else if(pid == 0)
		{
		//child process: recieve msg
			memset(buf,0,100);
			if(recv(clientfd,buf,100,0)<=0)
			{
				perror("recv:");
				close(clientfd);
				raise(SIGSTOP);
				exit(1);
			}
			printf("%s\n",buf);
		}
		else perror("fork");
	}

	//---------------------------

	close(clientfd);
	free(buf_r);
	free(buf);
	buf = NULL;
	buf_r = NULL;
	return 0;
}

