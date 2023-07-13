#include <iostream>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <time.h>  
#pragma comment(lib, "ws2_32.lib")

//  __FILE__ 获取源文件的相对路径和名字
//  __LINE__ 获取该行代码在文件中的行号
//  __func__ 或 __FUNCTION__ 获取函数名
#define LOGI(format, ...)  fprintf(stderr,"[INFO] [%s:%d]:%s() " format "\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOGE(format, ...)  fprintf(stderr,"[ERROR] [%s:%d]:%s() " format "\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)

int main(int argc, char** argv) {
	
	const char * serverIp = "127.0.0.1";
	const int serverPort = 8080;

	//const int clientPort = 8011;// 可以指定客户端端口，也可以不指定

	LOGI("TcpClient start,serverIp=%s,serverPort=%d", serverIp,serverPort);


	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		LOGE("WSAStartup error");
		return -1;
	}

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		LOGE("create socket error");
		WSACleanup();
		return -1;
	}
	int on = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

	/*
	sockaddr_in client_addr;
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	//client_addr.sin_port = htons(clientPort);
	if (bind(fd, (LPSOCKADDR)&client_addr, sizeof(SOCKADDR)) == -1)
	{
		LOGE("socket bind error");
		WSACleanup();
		return -1;
	}
	*/

	//设置 server_addr
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(serverPort);
	//server_addr.sin_addr.s_addr = inet_addr(serverIp);
	inet_pton(AF_INET, serverIp, &server_addr.sin_addr);

	if (connect(fd, (struct sockaddr*)&server_addr, sizeof(sockaddr_in)) == -1)
	{
		LOGE("socket connect error");
		return -1;
	}
	LOGI("fd=%d connect success",fd);

	char buf[10000];
	int size;
	uint64_t  totalSize = 0;
	time_t  t1 = time(NULL);

	while (true) {
		size = recv(fd, buf, sizeof(buf), 0);
		if (size <= 0) {
			break;
		}
		totalSize += size;

		if (totalSize > 6291456)/* 62914560=60*1024*1024=60mb*/
		{
			time_t t2 = time(NULL);
			if (t2 - t1 > 0) {
				uint64_t speed = totalSize / 1024 / 1024 / (t2 - t1);
				printf("fd=%d,size=%d,totalSize=%llu,speed=%lluMbps\n", fd,size, totalSize, speed);
				totalSize = 0;
				t1 = time(NULL);
			}
		}
	}

	LOGE("fd=%d disconnect", fd);

	closesocket(fd);
	WSACleanup();
	return 0;
}