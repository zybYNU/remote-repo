#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAXEVENTS 100

// ��socket����Ϊ�������ķ�ʽ��
int setnonblocking(int sockfd);

// ��ʼ������˵ļ����˿ڡ�
int initserver(int port);

int main(int argc,char *argv[])
{
  if (argc != 2)
  {
    printf("usage:./tcpepoll port\n"); return -1;
  }

  // ��ʼ����������ڼ�����socket��
  int listensock = initserver(atoi(argv[1]));
  printf("listensock=%d\n",listensock);

  if (listensock < 0)
  {
    printf("initserver() failed.\n"); return -1;
  }

  int epollfd;

  char buffer[1024];
  memset(buffer,0,sizeof(buffer));

  // ����һ��������
  epollfd = epoll_create(1);

  // ��Ӽ����������¼�
  struct epoll_event ev;
  ev.data.fd = listensock;
  ev.events = EPOLLIN;
  epoll_ctl(epollfd,EPOLL_CTL_ADD,listensock,&ev);

  while (1)
  {
    struct epoll_event events[MAXEVENTS]; // ������¼������Ľṹ���顣

    // �ȴ����ӵ�socket���¼�������
    int infds = epoll_wait(epollfd,events,MAXEVENTS,-1);
    // printf("epoll_wait infds=%d\n",infds);

    // ����ʧ�ܡ�
    if (infds < 0)
    {
      printf("epoll_wait() failed.\n"); perror("epoll_wait()"); break;
    }

    // ��ʱ��
    if (infds == 0)
    {
      printf("epoll_wait() timeout.\n"); continue;
    }

    // �������¼������Ľṹ���顣
    for (int ii=0;ii<infds;ii++)
    {
      if ((events[ii].data.fd == listensock) &&(events[ii].events & EPOLLIN))
      {
        // ��������¼�����listensock����ʾ���µĿͻ�����������
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int clientsock = accept(listensock,(struct sockaddr*)&client,&len);
        if (clientsock < 0)
        {
          printf("accept() failed.\n"); continue;
        }

        // ���µĿͻ�����ӵ�epoll�С�
        memset(&ev,0,sizeof(struct epoll_event));
        ev.data.fd = clientsock;
        ev.events = EPOLLIN;
        epoll_ctl(epollfd,EPOLL_CTL_ADD,clientsock,&ev);

        printf ("client(socket=%d) connected ok.\n",clientsock);

        continue;
      }
      else if (events[ii].events & EPOLLIN)
      {
        // �ͻ��������ݹ�����ͻ��˵�socket���ӱ��Ͽ���
        char buffer[1024];
        memset(buffer,0,sizeof(buffer));

        // ��ȡ�ͻ��˵����ݡ�
        ssize_t isize=read(events[ii].data.fd,buffer,sizeof(buffer));

        // �����˴����socket���Է��رա�
        if (isize <=0)
        {
          printf("client(eventfd=%d) disconnected.\n",events[ii].data.fd);

          // ���ѶϿ��Ŀͻ��˴�epoll��ɾ����
          memset(&ev,0,sizeof(struct epoll_event));
          ev.events = EPOLLIN;
          ev.data.fd = events[ii].data.fd;
          epoll_ctl(epollfd,EPOLL_CTL_DEL,events[ii].data.fd,&ev);
          close(events[ii].data.fd);
          continue;
        }

        printf("recv(eventfd=%d,size=%d):%s\n",events[ii].data.fd,isize,buffer);

        // ���յ��ı��ķ��ظ��ͻ��ˡ�
        write(events[ii].data.fd,buffer,strlen(buffer));
      }
    }
  }

  close(epollfd);

  return 0;
}

// ��ʼ������˵ļ����˿ڡ�
int initserver(int port)
{
  int sock = socket(AF_INET,SOCK_STREAM,0);
  if (sock < 0)
  {
    printf("socket() failed.\n"); return -1;
  }

  // Linux����
  int opt = 1; unsigned int len = sizeof(opt);
  setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,len);
  setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,&opt,len);

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(sock,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 )
  {
    printf("bind() failed.\n"); close(sock); return -1;
  }

  if (listen(sock,5) != 0 )
  {
    printf("listen() failed.\n"); close(sock); return -1;
  }

  return sock;
}

// ��socket����Ϊ�������ķ�ʽ��
int setnonblocking(int sockfd)
{  
  if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1)  return -1;

  return 0;  
}  
