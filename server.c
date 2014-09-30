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
#define MYPORT 5500 //宏定义，定义通信端口
#define BACKLOG 10  //宏定义，定义服务程序可以连接的最大客户数量
#define WELCOME  "|---------Welcome to the chat room!----------|"  //宏定义，当客户端连接到服务器时，向客户发送此欢迎字符

//转换函数，将int类型转换成char*类型
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
//得到当前系统的时间
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
//创建共享存储区
key_t shm_create()
{
key_t shmid;
if((shmid = shmget(IPC_PRIVATE,1024,PERM)) == -1)
{
fprintf(stderr,"Create Share Memory Error:%s\n\a",strerror(errno));
exit(1);
}
return shmid;
}
//端口绑定函数，创建套件字，并绑定到指定端口
int bindPort(unsigned short int port)
{
int sockfd;
struct sockaddr_in my_addr;
sockfd = socket(AF_INET,SOCK_STREAM,0);//创建基于六套接字
my_addr.sin_family = AF_INET; //IPv4协议族
my_addr.sin_port = htons(port);//端口转换
my_addr.sin_addr.s_addr = INADDR_ANY;
bzero(&(my_addr.sin_zero),0);
if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr)) == -1)
{
perror("bind");
exit(1);
}
printf("bind success!\n");
return sockfd;
}
void write_in_file(char* buf_in)
{
FILE *fp;
if((fp = fopen("chat_log","a+")) == NULL)
{
printf("文件不能打开！\n");
exit(1);
}
fprintf(fp,"%s\n",buf_in);
fclose(fp);
}
int main(int argc,char *argv[])
{
socklen_t sockfd,clientfd,recvbytes;//定义监听套接字、客户套接字
socklen_t sin_size;
pid_t pid,ppid; //定义父子线程标记变量 pid_t == Process ID_Type 宏定义insigned int 类型
char *buf,*r_addr,*w_addr,*temp,*time_str; //定义临时存储区
struct sockaddr_in their_addr; //定义地址结构
key_t shmid;
shmid = shm_create(); //创建共享存储区
temp = (char*)malloc(255);
time_str = (char*)malloc(20);
sockfd = bindPort(MYPORT); //绑定端口
while(1)
{
if(listen(sockfd,BACKLOG) == -1) //指定端口上监听
{
perror("listen");
exit(1);
}
printf("listening......\n");
if((clientfd = accept(sockfd,(struct sockaddr*)&their_addr,&sin_size)) == -1)
{
perror("accept");
exit(1);
}
printf("accept from: %s\n",inet_ntoa(their_addr.sin_addr));
send(clientfd,WELCOME,strlen(WELCOME),0);//发送问候消息
buf = (char*)malloc(255);
ppid = fork();//创建子进程
if(ppid == 0)
{
pid = fork(); //创建子进程
while(1)
{
if(pid >0)
{
//父进程用于接收信息
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
printf("接收的：%s\n",buf);
write_in_file(buf);
}
else if(pid == 0)
{
//子进程用于发送信息
//scanf("%s",buf);
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
else
perror("fork");
}
}
}
printf("------------------------\n");
free(buf);
close(sockfd);
close(clientfd);
return 0;
}
