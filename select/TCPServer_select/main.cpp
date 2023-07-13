#include <iostream>
#include <time.h>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

static std::string getTime() {
	const char* time_fmt = "%Y-%m-%d %H:%M:%S";
	time_t t = time(nullptr); // time(nullptr) 函数返回从系统时钟获取的当前时间的秒数
	char time_str[64];
	strftime(time_str, sizeof(time_str), time_fmt, localtime(&t)); // localtime 函数将 time_t 类型的时间值转换为包含年、月、日、时、分、秒等信息的 tm 结构体指针，以便后续格式化

	return time_str;
}
//  __FILE__ 获取源文件的相对路径和名字
//  __LINE__ 获取该行代码在文件中的行号
//  __func__ 或 __FUNCTION__ 获取函数名

#define LOGI(format, ...)  fprintf(stderr,"[INFO]%s [%s:%d %s()] " format "\n", getTime().data(),__FILE__,__LINE__,__func__ ,##__VA_ARGS__)
#define LOGE(format, ...)  fprintf(stderr,"[ERROR]%s [%s:%d %s()] " format "\n",getTime().data(),__FILE__,__LINE__,__func__ ,##__VA_ARGS__)
/*
以下select的实现虽然大幅提高了线程利用率，但是我们发现，select只能得出就绪文件描述符数量，具体是哪些，需要我们轮询进行判断，轮询的时间复杂度是O(n)，当并发量很大时，这个效率也不够了，那么有没有什么解决办法呢？

基于事件驱动！
也就是poll、epoll模型(主要是epoll，poll中仍然需要轮询，但效率还是有一定的提升的，虽然不多。poll每次轮询都有将事件数组复制到内核上的一步，而epoll采用回调的方式，因此效率更高的主要还是epoll。poll与epoll都达到了内核级别的委托)

*/

int main() {
	const char* ip = "127.0.0.1";
	uint16_t port = 8080;

	LOGI("TcpServer_select tcp://%s:%d", ip, port);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		LOGE("WSAStartup Error");
		return -1;
	}
	
	int serverFd = socket(AF_INET, SOCK_STREAM, 0);//创建描述符
	if (serverFd < 0) {
		LOGI("create socket error");
		return -1;
	}
	int ret;
	//unsigned long ul = 1;
	//ret = ioctlsocket(serverFd, FIONBIO, (unsigned long*)&ul);
	//if (ret == SOCKET_ERROR) {
	//	LOGE("设置非阻塞失败");
	//	return -1;
	//}

	int on = 1;
	setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)); // 设置为套接字为可复用，保证多个进程或线程需要绑定到相同的地址和端口，以实现负载均衡或并发处理

	// bind
	SOCKADDR_IN server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	//server_addr.sin_addr.s_addr = inet_addr("192.168.2.61");
	server_addr.sin_port = htons(port);

	if (bind(serverFd, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		LOGI("socket bind error");
		return -1;
	}
	// listen
	if (listen(serverFd, 10) < 0)
	{
		LOGE("socket listen error");
		return -1;
	}

	char recvBuf[1000] = { 0 }; //接收缓冲区
	int  recvBufLen = 1000;
	int  recvLen = 0;

	int max_fd = 0;
	fd_set readfds; // 定义了一个文件描述符集合 readfds，用于存储要监视的文件描述符 注：fd_set是文件描述符集合，通过操作 fd_set 可以有效地管理和操作要监视的文件描述符
	FD_ZERO(&readfds); // 将 readfds 文件描述符集合清空，确保其中不包含任何文件描述符

	//将sockFd添加进入集合内，并更新最大文件描述符
	FD_SET(serverFd, &readfds);
	max_fd = max_fd > serverFd ? max_fd : serverFd;
	/*
	76~81行代码用于设置 select  函数所需的文件描述符集合 readfds 和最大文件描述符数 max_fd 
	1.max_fd 被初始化为 0，它将用于记录当前文件描述符集合中的最大文件描述符数。
	2.readfds 是一个文件描述符集合，用于存储要监视的文件描述符。使用 FD_ZERO 宏将 readfds 清空，确保其中不包含任何文件描述符。
	3.使用 FD_SET 宏将 serverFd（服务器的监听套接字）添加到 readfds 文件描述符集合中。这样，select 函数将监视 serverFd 是否有可读事件发生。
	4.通过比较 serverFd 和 max_fd，将较大的值赋给 max_fd，以确保 max_fd 记录了文件描述符集合中的最大文件描述符数。
	以上，readfds 文件描述符集合就准备好了，可以作为参数传递给 select 函数，用于等待文件描述符的就绪状态。
	*/

	struct timeval timeout; // 设置select函数的超时时间
	timeout.tv_sec = 0;// 秒
	timeout.tv_usec = 0;//微秒
	
	char sendBuf[10000]; // 发送缓冲区
	int  sendBufLen = 10000;
	memset(sendBuf, 0, sendBufLen); 

	while (true) {
		//printf("loop...\n");
		fd_set readfds_temp;
		FD_ZERO(&readfds_temp);

		readfds_temp = readfds;
		ret = select(max_fd + 1, &readfds_temp, nullptr, nullptr, &timeout); // 这里timeout为0，也即超时时间为 0，表示在没有就绪文件描述符的情况下立即返回
		/*
		int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
		nfds：要监视的文件描述符的最大值加一。即需要监视的文件描述符集合中的最大文件描述符加一。
		readfds：可读事件的文件描述符集合。如果文件描述符集合中的文件描述符有可读事件发生，则返回时会将相应的文件描述符从该集合中清除。
		writefds：可写事件的文件描述符集合。如果文件描述符集合中的文件描述符有可写事件发生，则返回时会将相应的文件描述符从该集合中清除。
		exceptfds：异常事件的文件描述符集合。如果文件描述符集合中的文件描述符有异常事件发生，则返回时会将相应的文件描述符从该集合中清除。
		timeout：超时时间，指定了函数的阻塞时间。它是一个 struct timeval 结构体指针，用于设置阻塞的秒数和微秒数。有以下两种情况：
			如果 timeout 设置为 nullptr，select 函数会一直阻塞，直到有文件描述符就绪。
			如果 timeout 设置为一个指向已初始化的 struct timeval 结构体的指针，select 函数会在指定的时间内阻塞，超时后返回。
		
		select 函数的返回值表示就绪的文件描述符数量，即发生了可读、可写或异常事件的文件描述符数量。返回值有以下几种情况：
		1.返回值为负数，表示 select 函数执行出错，发生了错误。
		2.返回值为 0，表示超时，即在指定的时间内没有文件描述符就绪。
		3.返回值大于 0，表示有文件描述符就绪，可以对就绪的文件描述符进行相应的操作。
		select 函数是一个阻塞函数，当有文件描述符就绪或超时时才会返回。它可以在单个线程中同时处理多个文件描述符的事件，实现了非阻塞的 I/O 操作和事件驱动的编程模型。

		注：这里的文件描述符指的就是套接字的描述符。

		那作为套接字的描述符，为什么还要区分为可读、可写和异常三种状态？意义是什么？
		一个参考：区分套接字的可读、可写和异常状态的意义在于实现对套接字的多种事件的监视和处理。
			可读状态：表示套接字上有数据可读取。当套接字接收缓冲区中有数据可读取时，可读状态会被设置。此时可以调用读取数据的操作来处理接收到的数据。
			可写状态：表示套接字可以发送数据。当套接字发送缓冲区有足够的空间可以发送数据时，可写状态会被设置。此时可以调用写入数据的操作来发送数据。
			异常状态：表示套接字发生异常情况。例如，套接字连接断开、收到带外数据等情况会导致套接字的异常状态被设置。此时可以进行相应的异常处理，如关闭套接字或重新建立连接。
			通过对套接字的可读、可写和异常状态进行监视，可以实现对套接字的多种事件的及时处理。例如，当一个套接字变为可读状态时，即有数据到达，可以立即读取数据并进行处理。同样地，当一个套接字变为可写状态时，即发送缓冲区有空间可以发送数据，可以立即进行数据发送操作。而异常状态的监视可以帮助及时处理套接字发生的异常情况，保证网络通信的稳定性和可靠性。
		这种可读、可写与异常，有点“一切皆文件的味道”在里面？
		*/

		if (ret < 0) {
			// LOGE("未检测到活跃fd"); 
		}
		else {
			// 每个进程默认打开3个文件描述符
			// 0,1,2 其中0代表标准输入流，1代表标准输出流，2代表标准错误流

			for (int fd = 3; fd < max_fd + 1; fd++)
			{
				if (FD_ISSET(fd, &readfds_temp)) {
					LOGI("fd=%d，触发可读事件", fd);

					if (fd == serverFd) {// 如果是监听套接字就绪了则表示有新的客户端连接请求
						int clientFd;
						if ((clientFd = accept(serverFd, NULL, NULL)) == -1)
						{
							LOGE("accept error");
						}
						LOGI("发现新连接：clientFd=%d", clientFd);

						// 如果有客户端连接将产生的新的文件描述符添加到集合中,并更新最大文件描述符
						FD_SET(clientFd, &readfds);
						max_fd = max_fd > clientFd ? max_fd : clientFd;
					}
					else {// 如果就绪的文件描述符不是监听套接字，则表示是已连接的客户端套接字，也即着客户端发送了消息到服务器
						// memset(recvBuf, 0, recvBufLen);
						recvLen = recv(fd, recvBuf, recvBufLen, 0);// 接收消息咯

						if (recvLen <= 0) // 如果 recvLen 小于等于 0，表示接收数据出现了错误或连接关闭
						{
							LOGE("fd=%d,recvLen=%d error", fd,recvLen);
							closesocket(fd);
							FD_CLR(fd, &readfds); // 从可读集合中删除
							continue;
						}
						else {
							LOGI("fd=%d,recvLen=%d success", fd, recvLen);
						}
					}
				}

			}
		}
	
		for (int i = 0; i < readfds.fd_count; i++) {
			int fd = readfds.fd_array[i];
			if (fd != serverFd) {
				// 客户端fd
		
				int size = send(fd, sendBuf, sendBufLen, 0); // 给每个非监听套接字发点0
				if (size < 0) { // 发送失败的情况
					LOGE("fd=%d,send error，错误码：%d", fd, WSAGetLastError());
					continue;
				}

			}
		}
	}

	if (serverFd > -1) { // 监听套接字有效则结束
		closesocket(serverFd);
		serverFd = -1;
	}
	WSACleanup();

	return 0;
}