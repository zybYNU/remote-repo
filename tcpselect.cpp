#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

// ��ʼ������˵ļ����˿ڡ�
int initserver(int port);

int main(int argc,char *argv[])
{
  if (argc != 2)
  {
    printf("usage: ./tcpselect port\n"); return -1;
  }

  // ��ʼ����������ڼ�����socket��
  int listensock = initserver(atoi(argv[1]));
  printf("listensock=%d\n",listensock);

  if (listensock < 0)
  {
    printf("initserver() failed.\n"); return -1;
  }

  fd_set readfdset;  // ���¼��ļ��ϣ���������socket�Ϳͻ�������������socket��
  int maxfd;  // readfdset��socket�����ֵ��

  // ��ʼ���ṹ�壬��listensock��ӵ������С�
  FD_ZERO(&readfdset);

  FD_SET(listensock,&readfdset);
  maxfd = listensock;

  while (1)
  {
    // ����select����ʱ����ı�socket���ϵ����ݣ�����Ҫ��socket���ϱ�����������һ����ʱ�ĸ�select��
    fd_set tmpfdset = readfdset;

    int infds = select(maxfd+1,&tmpfdset,NULL,NULL,NULL);
    // printf("select infds=%d\n",infds);

    // ����ʧ�ܡ�
    if (infds < 0)
    {
      printf("select() failed.\n"); perror("select()"); break;
    }

    // ��ʱ���ڱ������У�select�������һ������Ϊ�գ������ڳ�ʱ������������´��뻹�����š�
    if (infds == 0)
    {
      printf("select() timeout.\n"); continue;
    }

    // ��������鷢����socket�����������Ϳͻ������ӵ�socket��
    // �����ǿͻ��˵�socket�¼���ÿ�ζ�Ҫ�����������ϣ���Ϊ�����ж��socket���¼���
    for (int eventfd=0; eventfd <= maxfd; eventfd++)
    {
      if (FD_ISSET(eventfd,&tmpfdset)<=0) continue;

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

        // ���µĿͻ���socket���뼯�ϡ�
        FD_SET(clientsock,&readfdset);

        if (maxfd < clientsock) maxfd = clientsock;

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

          FD_CLR(eventfd,&readfdset);  // �Ӽ�������ȥ�ͻ��˵�socket��

          // ���¼���maxfd��ֵ��ע�⣬ֻ�е�eventfd==maxfdʱ����Ҫ���㡣
          if (eventfd == maxfd)
          {
            for (int ii=maxfd;ii>0;ii--)
            {
              if (FD_ISSET(ii,&readfdset))
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
