/*service.c
 * based on socket C
 * authored by Tank, 2014-11-23
 * Tank <derektanko@gmail.com>
 * It's just my homework~
 */

#include <stdio.h>
#include <stdlib.h>
#include <x86_64-linux-gnu/sys/types.h>
#include <x86_64-linux-gnu/sys/stat.h>
#include <netinet/in.h>
#include <x86_64-linux-gnu/sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <x86_64-linux-gnu/sys/ipc.h>
#include <errno.h>
#include <x86_64-linux-gnu/sys/shm.h>
#include <time.h>
#include <arpa/inet.h>

#define PERM S_IRUSR|S_IWUSR
#define MYPORT 5503 
#define BACKLOG 10  //max of client number
#define WELCOME  "\033[34m|---------Welcome to the chat room!----------|\033[0m"  //send this sentence to client when anyone comes into the chat room

//------All functions------
void itoa(int i,char*string);
void get_cur_time(char *time_str);
key_t shm_create();
int bindPort(unsigned short int port);
void write_in_file(char* buf_in);
int auth(char*USER,char*PASS);
//-------------------------

//------Main Function------
int main(int argc,char *argv[])
{
	socklen_t sockfd,clientfd,recvbytes;
	socklen_t sin_size;
	pid_t pid,ppid; 		//pid to sign the different process
	char *buf,*r_addr,*w_addr,*temp,*time_str,*USER,*PASS;
	struct sockaddr_in their_addr;
	key_t shmid;
	shmid = shm_create();
	temp = (char*)malloc(255);
	time_str = (char*)malloc(20);
	sockfd = bindPort(MYPORT);
	while(1)
	{
		if(listen(sockfd,BACKLOG) == -1)
		{
			perror("listen");
			exit(1);
		}
		printf("\033[32mlistening......\n\033[0m");
		if((clientfd = accept(sockfd,(struct sockaddr*)&their_addr,&sin_size)) == -1)
		{
			perror("accept");
			exit(1);
		}
		printf("\033[32maccept from: %s\n\033[0m",inet_ntoa(their_addr.sin_addr));
		send(clientfd,WELCOME,strlen(WELCOME),0);//send welcome msg
		buf = (char*)malloc(255);
		
		
		USER=(char*)malloc(sizeof(255));
		PASS=(char*)malloc(sizeof(255));
		memset(buf,0,255);
		recv(clientfd,buf,255,0);
		strcpy(USER,buf);
		printf("USER : %s\n",USER);
		memset(buf,0,255);
		recv(clientfd,buf,255,0);
		strcpy(PASS,buf);
		printf("PASS : %s\n",PASS);
		if(auth(USER,PASS))
		{
			printf("\033[31mThe login request of user \"%s\" was rejected!\n\033[0m",USER);
			send(clientfd,"FAILED",7,0);
			continue;
		}
		send(clientfd,"SUCCESS",8,0);
		printf("\033[32muser \"%s\" connected!\n\033[0m",USER);
		
		
		
		ppid = fork();
		if(ppid == 0)
		{
			pid = fork();
			while(1)
			{
				if(pid >0)
				{
				//parent process: recieve msg
					memset(buf,0,255);
					if((recvbytes = recv(clientfd,buf,255,0))<=0)
					{
						perror("recv1");
						close(clientfd);
						raise(SIGKILL);
						exit(1);
					}
					//write buf's data to share memory
					w_addr = shmat(shmid,0,0);
					memset(w_addr,'\0',1024);
					strncpy(w_addr,buf,1024);
					get_cur_time(time_str);
					strcat(buf,time_str);
					printf("->%s\n",buf);
					write_in_file(buf);
				}
				else if(pid == 0)
				{
				//child process: send msg
					sleep(1);
					r_addr = shmat(shmid,0,0);
					if(strcmp(temp,r_addr)!=0)
					{
						strcpy(temp,r_addr);
						get_cur_time(time_str);
						strcat(r_addr,time_str);
						if(send(clientfd,r_addr,strlen(r_addr),0) == -1)
						{
							perror("send");
						}
						memset(r_addr,'\0',1024);
						strcpy(r_addr,temp);
					}
				}
				else perror("fork");
			}
		}
	}
	printf("------------------------\n");
	free(buf);
	close(sockfd);
	close(clientfd);
	return 0;
}

//-----------------------

//------All functions declaration------

//int to ascii
void itoa(int i,char*string)
{
	int power,j;
	j=i;
	for(power=1;j>=10;j/=10)
	power*=10;
	for(;power>0;power/=10)
	{
		*string++='0'+i/power;
		i%=power;
	}
	*string='\0';
}

//get the system time
void get_cur_time(char *time_str)
{
	time_t timep;
	struct tm *p_curtime;
	char *time_tmp;
	time_tmp = (char*)malloc(2);
	memset(time_tmp,0,2);
	memset(time_str,0,20);
	time(&timep);
	p_curtime = localtime(&timep);
	strcat(time_str,"(");
	itoa(p_curtime->tm_hour,time_tmp);
	strcat(time_str,time_tmp);
	strcat(time_str,":");
	itoa(p_curtime->tm_min,time_tmp);
	strcat(time_str,time_tmp);
	strcat(time_str,":");
	itoa(p_curtime->tm_sec,time_tmp);
	strcat(time_str,time_tmp);
	strcat(time_str,")");
	free(time_tmp);
}

//create the share space
key_t shm_create()
{
	key_t shmid;
	if((shmid = shmget(IPC_PRIVATE,1024,PERM)) == -1)
	{
		fprintf(stderr,"\033[31mCreate Share Memory Error:%s\n\a\033[0m",strerror(errno));
		exit(1);
	}
	return shmid;
	}

//bind the port
int bindPort(unsigned short int port)
{
	int sockfd;
	struct sockaddr_in my_addr;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	my_addr.sin_family = AF_INET; 
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero),0);
	if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr)) == -1)
	{
		perror("bind");
		exit(1);
	}
	printf("\033[32mbind success!\n\033[0m");
	return sockfd;
}

//save chat log to file
void write_in_file(char* buf_in)
{
	FILE *fp;
	if((fp = fopen("chat_log","a+")) == NULL)
	{
		printf("\033[31mFailed to open the char log!\n\033[0m");
		exit(1);
	}
	fprintf(fp,"%s\n",buf_in);
	fclose(fp);
}

//login authentication
int auth(char*USER,char*PASS)
{
	FILE *userdb=fopen("users.db","r");
	if(userdb==NULL)
	{
		printf("\033[31mfailed to read Users-Database.\033[0m");
		exit(1);
	}
	char u[50],p[50];
	while(fscanf(userdb,"%s %s",u,p)!=EOF)
	if(!strcmp(USER,u)&&!strcmp(PASS,p))
	{
		fclose(userdb);
		return 0;
	}
	fclose(userdb);
	return 1;
}


//---------------------------
