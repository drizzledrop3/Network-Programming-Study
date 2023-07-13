#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

/*
以下代码使用了epoll来实现一个简单的基于事件驱动的TCP服务器。
1.代码创建了一个套接字 serverFd，并通过 bind 函数将其绑定到指定的 IP 地址和端口。然后，通过 listen 函数开始监听连接请求。
2.代码创建了一个 epoll 实例，通过 epoll_create 函数获取一个 epoll 文件描述符 epfd。然后，将监听套接字 serverFd 添加到 epoll 实例中，以便监听新的连接请求。
3.在进入主循环之前，代码定义了一个事件数组 events，用于存储 epoll_wait 函数返回的就绪事件。
4.进入主循环，使用 epoll_wait 函数监听事件的发生。当有事件就绪时，epoll_wait 函数返回，并将就绪事件存储在 events 数组中。通过遍历 events 数组，可以处理就绪的事件。
5.在循环中，首先判断就绪事件是来自监听套接字 serverFd 还是已连接套接字。如果是来自监听套接字 serverFd 的事件，表示有新的连接请求，使用 accept 函数接受连接，然后将新的连接套接字添加到 epoll 实例中。
6.如果是已连接套接字的事件，说明有数据可读或可写。在代码中，先处理读事件，通过 read 函数读取客户端发送的数据。如果读取到的数据长度小于等于0，表示客户端断开连接，关闭套接字，并从 epoll 实例中移除该套接字。如果读取到正常的数据，则进行相应的处理。
7.处理写事件，通过 send 函数向客户端发送数据。如果发送失败或发送的数据长度小于0，表示发送出错，关闭套接字，并从 epoll 实例中移除该套接字。
8.循环继续，等待下一轮的事件就绪。

整个流程是基于事件驱动的，通过 epoll 来监听套接字的事件，一旦有事件就绪，立即进行处理，从而实现高效的网络通信。
*/

int main() {
    // 1.创建套接字
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        perror("socket");
        exit(0);
    }

    // 2. 绑定 ip, port
    struct sockaddr_in addr;
    addr.sin_port = htons(8080);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(serverFd, (struct sockaddr *) &addr, sizeof(addr));
    if (ret == -1) {
        perror("bind");
        exit(0);
    }

    // 3. 监听
    ret = listen(serverFd, 100);
    if (ret == -1) {
        perror("listen");
        exit(0);
    }

    // 创建epoll树
    int epfd = epoll_create(1000);//有关size参数介绍：https://blog.csdn.net/zhoumuyu_yu/article/details/112472419
    /*
    epoll_create 是一个 Linux 下用于创建 epoll 实例的系统调用函数。它的原型如下：
    int epoll_create(int size);
    该函数创建一个 epoll 实例，并返回一个与该实例相关的文件描述符。参数 size 指定了该 epoll 实例能同时监听的文件描述符的数量上限。
     这个参数在新的内核版本中已经被忽略，因此可以将其设置为任意非负值（注意，得是非负值）。
    
    epoll_create 函数的主要作用是创建一个 epoll 实例，用于进行高效的 I/O 事件监听。
     在创建 epoll 实例之后，可以使用其他的 epoll 相关系统调用，如 epoll_ctl 和 epoll_wait，来对该实例进行操作。
    
    关于 epoll_create 函数的一些特点和注意事项：
    1. 每个 epoll 实例是独立的，与其他实例隔离。
    2. epoll 实例使用文件描述符进行标识和操作。
    3. epoll_create 函数创建的 epoll 实例在使用完毕后需要调用 close 函数进行关闭。
    4. epoll_create 函数返回的文件描述符可以用于后续的 epoll_ctl 和 epoll_wait 调用。
    5. epoll_create 函数的返回值为 -1 表示创建失败，此时可以通过 perror 函数输出错误信息。
    总而言之，epoll_create 是一个用于创建 epoll 实例的系统调用函数，用于提供高效的 I/O 事件监听机制。
     * */
    if (epfd == -1) {
        perror("epoll_create");
        /*
        perror 是一个标准库函数，用于将最近一次的系统错误信息输出到标准错误流（stderr）上。它的原型如下：
        void perror(const char *s);
        函数参数 s 是一个字符串，用于在输出错误信息之前显示。perror 函数会根据最近一次发生的系统错误，在标准错误流上输出一个形如 "s: 错误描述" 的字符串。
         其中，s 是传入的参数字符串，表示用户自定义的前缀信息。
        perror 函数常用于在程序中捕获和处理系统调用错误，以便在出现错误时输出有关错误的详细信息。
         它可以帮助开发人员更好地了解程序运行中发生的错误，并进行适当的处理或调试。

        perror 函数输出的错误信息通常包括错误号和错误描述，可以帮助定位和诊断错误。
        错误信息输出到标准错误流（stderr），与标准输出流（stdout）分开。
        在调用 perror 之前，通常需要先调用发生错误的系统调用，以便获取最近一次的系统错误信息。
        perror 只输出最近一次发生的系统错误，之后的错误会覆盖前面的错误信息。
         * */
        exit(0);
    }

    // 将监听lfd添加到树上
    struct epoll_event serverFdEvt;
    // 检测事件的初始化
    serverFdEvt.events = EPOLLIN; //指定感兴趣的事件类型 这里指定为EPOLLIN表明对可读事件感兴趣
    serverFdEvt.data.fd = serverFd; // data.fd 成员用于存储触发事件的文件描述符 这里就是服务器的监听套接字serverFd
    epoll_ctl(epfd, EPOLL_CTL_ADD, serverFd, &serverFdEvt);
    /*
    epoll_ctl 函数用于控制 epoll 实例中的事件，包括添加、修改和删除事件。它的函数原型如下：
    int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

    epfd 是 epoll 实例的文件描述符，表示要操作的 epoll 实例。

    op 是操作标志，用于指定要执行的操作类型，可以是以下值之一：
    EPOLL_CTL_ADD、EPOLL_CTL_MOD和EPOLL_CTL_DEL，含义见event参数中解释

    fd 是要操作的文件描述符。
    event 是一个指向 epoll_event 结构体的指针，表示要添加、修改或删除的事件及相关数据。
    在 epoll_ctl 函数中，根据传入的操作标志 op 的不同，可以进行以下操作：
    添加事件（EPOLL_CTL_ADD）：将指定的文件描述符 fd 添加到 epoll 实例 epfd 中，并关联指定的事件和数据。
    修改事件（EPOLL_CTL_MOD）：修改已经添加到 epoll 实例中的文件描述符 fd 上的事件和数据。
    删除事件（EPOLL_CTL_DEL）：从 epoll 实例 epfd 中删除指定的文件描述符 fd。
     * */
    struct epoll_event events[1024];

    int s1 = sizeof(events);
    int s2 = sizeof(events[0]);

    char sendBuf[10000];
    int sendBufLen = 10000;
    memset(sendBuf, 0, sendBufLen);

    // 开始检测
    while (true) {
        int nums = epoll_wait(epfd, events, s1 / s2, 1);// 超时时间为 1 milliseconds
        /*
        epoll_wait 是一个用于等待事件发生的函数，它是 Linux 中使用 epoll 模型的关键函数之一。它的函数原型如下：
        int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

        epfd：epoll 实例的文件描述符，用于指定要监听的事件集合。
        events：指向用于存储事件的结构体数组的指针。在函数调用之前，需要分配足够的空间用于存储事件。
        maxevents：events 数组的大小，即可以存储的最大事件数量。
        timeout：等待事件发生的超时时间，单位是毫秒。有以下几种情况：
            传入负数，表示无限等待，直到有事件发生。
            传入 0，表示立即返回，不等待事件。
            传入正数，表示等待指定的时间内有事件发生。

        函数返回值表示实际发生的事件数量，如果返回值为 0，则表示超时，没有事件发生。如果返回值为 -1，则表示出现错误，可以通过查看 errno 来获取具体的错误信息。
        在调用 epoll_wait 后，可以通过遍历 events 数组来处理每个发生的事件。每个事件包含了文件描述符和事件类型等信息，可以根据需要进行处理。
         * */
        printf("serverFd=%d,nums = %d\n", serverFd, nums);

        // 遍历状态变化的文件描述符集合
        for (int i = 0; i < nums; ++i) {
            int curfd = events[i].data.fd;

            printf("curfd=%d \n", curfd);

            if (curfd == serverFd) { // 新事件为监听套接字 则表明有新连接
                struct sockaddr_in conn_addr;
                socklen_t conn_addr_len = sizeof(addr);
                int connfd = accept(serverFd, (struct sockaddr *) &conn_addr, &conn_addr_len);

                printf("connfd=%d\n", connfd);
                if (connfd == -1) {
                    perror("accept\n");
//                    exit(0);
                    break;
                }
                // 将通信的fd挂到树上
                serverFdEvt.events = EPOLLIN | EPOLLOUT; // EPOLLIN | EPOLLOUT： 将事件类型设置为同时监听可读和可写事件。
//                serverFdEvt.events = EPOLLIN;
                serverFdEvt.data.fd = connfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &serverFdEvt);
            }
                // 通信
            else { // 为旧连接发出数据
                // 写事件触发 因为没有写事件，所以注释了
//                if(events[i].events & EPOLLOUT)
//                {
//                    continue;
//                }

                if (events[i].events & EPOLLIN) { //  & 是为了检查 events[i].events 中是否包含 EPOLLOUT 事件
                    char buf[128];
                    int count = read(curfd, buf, sizeof(buf));
                    if (count <= 0) { // count 小于等于 0，则表示读取数据时出现了错误或连接被断开
                        printf("client disconnect ...\n");
                        close(curfd);
                        // 从树上删除该节点
                        epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);

                        continue;
                    } else {
                        // 正常情况
                        printf("client say: %s\n", buf);
                    }
                }
                if (curfd > -1) {

                    int size = send(curfd, sendBuf, sendBufLen, 0);
                    if (size < 0) {
                        printf("curfd=%d,send error \n", curfd);
                        close(curfd);
                        // 从树上删除该节点
                        epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);
                        continue;
                    }

                }


            }
        }
    }

    close(serverFd);

    return 0;
}


