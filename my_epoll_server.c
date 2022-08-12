#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<pthread.h>
#include<sys/epoll.h>
#include<ctype.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#define Port 8080
#define MAX_EVENTS 1024
typedef void (*call_back)(int, void*); 
//再也不能再远端进行修改了
//
// 登陆用户信息
typedef struct user
{
    char usr_id[8];            // 用户ID    五位 UID
    char usr_name[256];        // 用户名
    char usr_key[40];          // 用户密码
    int  st;                   // 是否在线 0---- 离线     1---- 在线
}user_msg;

user_msg Users[MAX_EVENTS];    // 已注册的所有用户信息
int user_num;                  // 已注册用户信息的数量

//描述监听的文件描述符的相关信息的结构体    
typedef struct myevent_s       
{
    int fd;                     // 监听的文件描述符
    int events;                 // 对应监听的事件 EPOLLIN / EPOLLOUT
    call_back fun;              // 回调函数
    void *arg;                  // 上面回调函数的参3
    int status;                 // 是否在监听红黑树上, 1 --- 在, 0 --- 不在
    char buf[BUFSIZ];           // 读写缓冲区
    int len;                    // 本次从客户端读入缓冲区数据的长度
    long long last_active_time; // 该文件描述符最后在监听红黑树上的活跃时间
    user_msg um;                // 用户登陆的信息
    int log_step;               // 标记用户位于登陆的操作 0-- 未登陆  1 --- 输入账号  2 ---- 输入密码   3----- 成功登陆  4 --- 注册用户名  5 ------ 输入注册的密码   6 ------- 再次输入密码验证
}myevent_s;

int g_efd;                           // 监听红黑树的树根
myevent_s g_events[MAX_EVENTS + 1];  // 用于保存每个文件描述符信息的结构体的数组

// 数组模拟双链表写法, 链表保存当前在线的用户
#define ONLINE_MAX 100000
int online_fd[ONLINE_MAX], l[ONLINE_MAX], r[ONLINE_MAX], fd_pos[ONLINE_MAX], idx, online_num;

void sys_error(const char* ch)
{
	perror(ch);
	exit(1);
}


// 链表进行初始化 
void list_init()
{
    r[0] = 1, l[1] = 0;              // 下标0 表示链表左端点,  下标1表示链表右端点
    idx = 2;
	int i;
    for(i = 3; i < ONLINE_MAX; ++i) fd_pos[i] = -1;
}

void load_usermsg()
{
	char buf[1024];
	getcwd(buf,sizeof(buf));
	sprintf(buf+strlen(buf), "/user_msg");
	FILE* fp = fopen(buf,"r");
	if(fp == NULL) sys_error("load error");

	while(!feof(fp))
	{

	}
}

int main(int argc, char *argv[])
{
    list_init();                                        // 初始化链表
    load_usermsg();                                     // 加载已注册用户的信息
    int lfd = socket(AF_INET, SOCK_STREAM, 0);          // 创建监听套接字
    if(lfd == -1) sys_error("socket error");

    // 设置端口复用
    int opt = 1;
    if(setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) == -1) sys_error("setsockopt error");

    // 绑定客户端的地址ip和端口号 , 设置监听上限
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(Port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(lfd, (struct sockaddr*)&server_addr, sizeof server_addr) == -1) sys_error("bind error");
    if(listen(lfd, 128) == -1) sys_error("listen error");

    // 创建epoll的监听树根
    g_efd = epoll_create(MAX_EVENTS + 1);   
    if(g_efd == -1) sys_error("epoll_create error");
	int i;
    for(i = 0; i < MAX_EVENTS; ++i) g_events[i].status = 0;   // 将数组各个位置标记为空闲状态

	return 0;
}
