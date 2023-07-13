#include <iostream>
#include <time.h>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

static std::string getTime() {
	const char* time_fmt = "%Y-%m-%d %H:%M:%S";
	time_t t = time(nullptr); // time(nullptr) �������ش�ϵͳʱ�ӻ�ȡ�ĵ�ǰʱ�������
	char time_str[64];
	strftime(time_str, sizeof(time_str), time_fmt, localtime(&t)); // localtime ������ time_t ���͵�ʱ��ֵת��Ϊ�����ꡢ�¡��ա�ʱ���֡������Ϣ�� tm �ṹ��ָ�룬�Ա������ʽ��

	return time_str;
}
//  __FILE__ ��ȡԴ�ļ������·��������
//  __LINE__ ��ȡ���д������ļ��е��к�
//  __func__ �� __FUNCTION__ ��ȡ������

#define LOGI(format, ...)  fprintf(stderr,"[INFO]%s [%s:%d %s()] " format "\n", getTime().data(),__FILE__,__LINE__,__func__ ,##__VA_ARGS__)
#define LOGE(format, ...)  fprintf(stderr,"[ERROR]%s [%s:%d %s()] " format "\n",getTime().data(),__FILE__,__LINE__,__func__ ,##__VA_ARGS__)
/*
����select��ʵ����Ȼ���������߳������ʣ��������Ƿ��֣�selectֻ�ܵó������ļ���������������������Щ����Ҫ������ѯ�����жϣ���ѯ��ʱ�临�Ӷ���O(n)�����������ܴ�ʱ�����Ч��Ҳ�����ˣ���ô��û��ʲô����취�أ�

�����¼�������
Ҳ����poll��epollģ��(��Ҫ��epoll��poll����Ȼ��Ҫ��ѯ����Ч�ʻ�����һ���������ģ���Ȼ���ࡣpollÿ����ѯ���н��¼����鸴�Ƶ��ں��ϵ�һ������epoll���ûص��ķ�ʽ�����Ч�ʸ��ߵ���Ҫ����epoll��poll��epoll���ﵽ���ں˼����ί��)

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
	
	int serverFd = socket(AF_INET, SOCK_STREAM, 0);//����������
	if (serverFd < 0) {
		LOGI("create socket error");
		return -1;
	}
	int ret;
	//unsigned long ul = 1;
	//ret = ioctlsocket(serverFd, FIONBIO, (unsigned long*)&ul);
	//if (ret == SOCKET_ERROR) {
	//	LOGE("���÷�����ʧ��");
	//	return -1;
	//}

	int on = 1;
	setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)); // ����Ϊ�׽���Ϊ�ɸ��ã���֤������̻��߳���Ҫ�󶨵���ͬ�ĵ�ַ�Ͷ˿ڣ���ʵ�ָ��ؾ���򲢷�����

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

	char recvBuf[1000] = { 0 }; //���ջ�����
	int  recvBufLen = 1000;
	int  recvLen = 0;

	int max_fd = 0;
	fd_set readfds; // ������һ���ļ����������� readfds�����ڴ洢Ҫ���ӵ��ļ������� ע��fd_set���ļ����������ϣ�ͨ������ fd_set ������Ч�ع���Ͳ���Ҫ���ӵ��ļ�������
	FD_ZERO(&readfds); // �� readfds �ļ�������������գ�ȷ�����в������κ��ļ�������

	//��sockFd��ӽ��뼯���ڣ�����������ļ�������
	FD_SET(serverFd, &readfds);
	max_fd = max_fd > serverFd ? max_fd : serverFd;
	/*
	76~81�д����������� select  ����������ļ����������� readfds ������ļ��������� max_fd 
	1.max_fd ����ʼ��Ϊ 0���������ڼ�¼��ǰ�ļ������������е�����ļ�����������
	2.readfds ��һ���ļ����������ϣ����ڴ洢Ҫ���ӵ��ļ���������ʹ�� FD_ZERO �꽫 readfds ��գ�ȷ�����в������κ��ļ���������
	3.ʹ�� FD_SET �꽫 serverFd���������ļ����׽��֣���ӵ� readfds �ļ������������С�������select ���������� serverFd �Ƿ��пɶ��¼�������
	4.ͨ���Ƚ� serverFd �� max_fd�����ϴ��ֵ���� max_fd����ȷ�� max_fd ��¼���ļ������������е�����ļ�����������
	���ϣ�readfds �ļ����������Ͼ�׼�����ˣ�������Ϊ�������ݸ� select ���������ڵȴ��ļ��������ľ���״̬��
	*/

	struct timeval timeout; // ����select�����ĳ�ʱʱ��
	timeout.tv_sec = 0;// ��
	timeout.tv_usec = 0;//΢��
	
	char sendBuf[10000]; // ���ͻ�����
	int  sendBufLen = 10000;
	memset(sendBuf, 0, sendBufLen); 

	while (true) {
		//printf("loop...\n");
		fd_set readfds_temp;
		FD_ZERO(&readfds_temp);

		readfds_temp = readfds;
		ret = select(max_fd + 1, &readfds_temp, nullptr, nullptr, &timeout); // ����timeoutΪ0��Ҳ����ʱʱ��Ϊ 0����ʾ��û�о����ļ����������������������
		/*
		int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
		nfds��Ҫ���ӵ��ļ������������ֵ��һ������Ҫ���ӵ��ļ������������е�����ļ���������һ��
		readfds���ɶ��¼����ļ����������ϡ�����ļ������������е��ļ��������пɶ��¼��������򷵻�ʱ�Ὣ��Ӧ���ļ��������Ӹü����������
		writefds����д�¼����ļ����������ϡ�����ļ������������е��ļ��������п�д�¼��������򷵻�ʱ�Ὣ��Ӧ���ļ��������Ӹü����������
		exceptfds���쳣�¼����ļ����������ϡ�����ļ������������е��ļ����������쳣�¼��������򷵻�ʱ�Ὣ��Ӧ���ļ��������Ӹü����������
		timeout����ʱʱ�䣬ָ���˺���������ʱ�䡣����һ�� struct timeval �ṹ��ָ�룬��������������������΢���������������������
			��� timeout ����Ϊ nullptr��select ������һֱ������ֱ�����ļ�������������
			��� timeout ����Ϊһ��ָ���ѳ�ʼ���� struct timeval �ṹ���ָ�룬select ��������ָ����ʱ������������ʱ�󷵻ء�
		
		select �����ķ���ֵ��ʾ�������ļ��������������������˿ɶ�����д���쳣�¼����ļ�����������������ֵ�����¼��������
		1.����ֵΪ��������ʾ select ����ִ�г��������˴���
		2.����ֵΪ 0����ʾ��ʱ������ָ����ʱ����û���ļ�������������
		3.����ֵ���� 0����ʾ���ļ����������������ԶԾ������ļ�������������Ӧ�Ĳ�����
		select ������һ�����������������ļ�������������ʱʱ�Ż᷵�ء��������ڵ����߳���ͬʱ�������ļ����������¼���ʵ���˷������� I/O �������¼������ı��ģ�͡�

		ע��������ļ�������ָ�ľ����׽��ֵ���������

		����Ϊ�׽��ֵ���������Ϊʲô��Ҫ����Ϊ�ɶ�����д���쳣����״̬��������ʲô��
		һ���ο��������׽��ֵĿɶ�����д���쳣״̬����������ʵ�ֶ��׽��ֵĶ����¼��ļ��Ӻʹ���
			�ɶ�״̬����ʾ�׽����������ݿɶ�ȡ�����׽��ֽ��ջ������������ݿɶ�ȡʱ���ɶ�״̬�ᱻ���á���ʱ���Ե��ö�ȡ���ݵĲ�����������յ������ݡ�
			��д״̬����ʾ�׽��ֿ��Է������ݡ����׽��ַ��ͻ��������㹻�Ŀռ���Է�������ʱ����д״̬�ᱻ���á���ʱ���Ե���д�����ݵĲ������������ݡ�
			�쳣״̬����ʾ�׽��ַ����쳣��������磬�׽������ӶϿ����յ��������ݵ�����ᵼ���׽��ֵ��쳣״̬�����á���ʱ���Խ�����Ӧ���쳣������ر��׽��ֻ����½������ӡ�
			ͨ�����׽��ֵĿɶ�����д���쳣״̬���м��ӣ�����ʵ�ֶ��׽��ֵĶ����¼��ļ�ʱ�������磬��һ���׽��ֱ�Ϊ�ɶ�״̬ʱ���������ݵ������������ȡ���ݲ����д���ͬ���أ���һ���׽��ֱ�Ϊ��д״̬ʱ�������ͻ������пռ���Է������ݣ����������������ݷ��Ͳ��������쳣״̬�ļ��ӿ��԰�����ʱ�����׽��ַ������쳣�������֤����ͨ�ŵ��ȶ��ԺͿɿ��ԡ�
		���ֿɶ�����д���쳣���е㡰һ�н��ļ���ζ���������棿
		*/

		if (ret < 0) {
			// LOGE("δ��⵽��Ծfd"); 
		}
		else {
			// ÿ������Ĭ�ϴ�3���ļ�������
			// 0,1,2 ����0�����׼��������1�����׼�������2�����׼������

			for (int fd = 3; fd < max_fd + 1; fd++)
			{
				if (FD_ISSET(fd, &readfds_temp)) {
					LOGI("fd=%d�������ɶ��¼�", fd);

					if (fd == serverFd) {// ����Ǽ����׽��־��������ʾ���µĿͻ�����������
						int clientFd;
						if ((clientFd = accept(serverFd, NULL, NULL)) == -1)
						{
							LOGE("accept error");
						}
						LOGI("���������ӣ�clientFd=%d", clientFd);

						// ����пͻ������ӽ��������µ��ļ���������ӵ�������,����������ļ�������
						FD_SET(clientFd, &readfds);
						max_fd = max_fd > clientFd ? max_fd : clientFd;
					}
					else {// ����������ļ����������Ǽ����׽��֣����ʾ�������ӵĿͻ����׽��֣�Ҳ���ſͻ��˷�������Ϣ��������
						// memset(recvBuf, 0, recvBufLen);
						recvLen = recv(fd, recvBuf, recvBufLen, 0);// ������Ϣ��

						if (recvLen <= 0) // ��� recvLen С�ڵ��� 0����ʾ�������ݳ����˴�������ӹر�
						{
							LOGE("fd=%d,recvLen=%d error", fd,recvLen);
							closesocket(fd);
							FD_CLR(fd, &readfds); // �ӿɶ�������ɾ��
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
				// �ͻ���fd
		
				int size = send(fd, sendBuf, sendBufLen, 0); // ��ÿ���Ǽ����׽��ַ���0
				if (size < 0) { // ����ʧ�ܵ����
					LOGE("fd=%d,send error�������룺%d", fd, WSAGetLastError());
					continue;
				}

			}
		}
	}

	if (serverFd > -1) { // �����׽�����Ч�����
		closesocket(serverFd);
		serverFd = -1;
	}
	WSACleanup();

	return 0;
}