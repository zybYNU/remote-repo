#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

// ulimit -n
#define MAXNFDS  1024

// ��ʼ������˵ļ����˿ڡ�
int initserver(int port);

int main(int argc,char *argv[])
{
  if (argc != 2)
  {
    printf("usage: ./tcppoll port\n"); return -1;
  }

  // ��ʼ����������ڼ�����socket��
  int listensock = initserver(atoi(argv[1]));
  printf("listensock=%d\n",listensock);

  if (listensock < 0)
  {
    printf("initserver() failed.\n"); return -1;
  }

  int maxfd;   // fds��������Ҫ���ӵ�socket�Ĵ�С��
  struct pollfd fds[MAXNFDS];  // fds�����Ҫ���ӵ�socket��

  for (int ii=0;ii<MAXNFDS;ii++) fds[ii].fd=-1; // ��ʼ�����飬��ȫ����fd����Ϊ-1��

  // ��listensock��ӵ������С�
  fds[listensock].fd=listensock;
  fds[listensock].events=POLLIN;  // �����ݿɶ��¼��������¿ͻ��˵����ӡ��ͻ���socket�����ݿɶ��Ϳͻ���socket�Ͽ����������
  maxfd=listensock;

  while (1)
  {
    int infds = poll(fds,maxfd+1,5000);
    // printf("poll infds=%d\n",infds);

    // ����ʧ�ܡ�
    if (infds < 0)
    {
      printf("poll() failed.\n"); perror("poll():"); break;
    }

    // ��ʱ��
    if (infds == 0)
    {
      printf("poll() timeout.\n"); continue;
    }

    // ��������鷢����socket�����������Ϳͻ������ӵ�socket��
    // �����ǿͻ��˵�socket�¼���ÿ�ζ�Ҫ�����������ϣ���Ϊ�����ж��socket���¼���
    for (int eventfd=0; eventfd <= maxfd; eventfd++)
    {
      if (fds[eventfd].fd<0) continue;

      if ((fds[eventfd].revents&POLLIN)==0) continue;

      fds[eventfd].revents=0;  // �Ȱ�revents��ա�

      if (eventfd==listensock)
      {
        // ��������¼�����listensock����ʾ���µĿͻ�����������
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int clientsock = accept(listensock,(struct sockaddr*)&client,&len);
        if (clientsock < 0)
        {
          printf("accept() failed.\n"); continue;
        }

        printf ("client(socket=%d) connected ok.\n",clientsock);

        if (clientsock>MAXNFDS)
        {    
          printf("clientsock(%d)>MAXNFDS(%d)\n",clientsock,MAXNFDS); close(clientsock); continue;
        }

        fds[clientsock].fd=clientsock;
        fds[clientsock].events=POLLIN; 
        fds[clientsock].revents=0; 
        if (maxfd < clientsock) maxfd = clientsock;

        printf("maxfd=%d\n",maxfd);
        continue;
      }
      else 
      {
        // �ͻ��������ݹ�����ͻ��˵�socket���ӱ��Ͽ���
        char buffer[1024];
        memset(buffer,0,sizeof(buffer));

        // ��ȡ�ͻ��˵����ݡ�
        ssize_t isize=read(eventfd,buffer,sizeof(buffer));

        // �����˴����socket���Է��رա�
        if (isize <=0)
        {
          printf("client(eventfd=%d) disconnected.\n",eventfd);

          close(eventfd);  // �رտͻ��˵�socket��

          fds[eventfd].fd=-1;

          // ���¼���maxfd��ֵ��ע�⣬ֻ�е�eventfd==maxfdʱ����Ҫ���㡣
          if (eventfd == maxfd)
          {
            for (int ii=maxfd;ii>0;ii--)
            {
              if ( fds[ii].fd != -1)
              {
                maxfd = ii; break;
              }
            }

            printf("maxfd=%d\n",maxfd);
          }

          continue;
        }

        printf("recv(eventfd=%d,size=%d):%s\n",eventfd,isize,buffer);

        // ���յ��ı��ķ��ظ��ͻ��ˡ�
        write(eventfd,buffer,strlen(buffer));
      }
    }
  }

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
