#include <stdint.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

//  __FILE__ ��ȡԴ�ļ������·��������
//  __LINE__ ��ȡ���д������ļ��е��к�
//  __func__ �� __FUNCTION__ ��ȡ������

#define LOGI(format, ...)  fprintf(stderr,"[INFO] [%s:%d]:%s() " format "\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOGE(format, ...)  fprintf(stderr,"[ERROR] [%s:%d]:%s() " format "\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)
/*
�����˺�(LOGI)�����ڷ�������������Ϣ�����ļ������кš��������Լ��Զ����ʽ����־��Ϣ��
*/

/*
������linux����windows��һ�����̿��õ��ڴ�ռ䶼�����ޣ�����windows�ڴ�ռ�������2G��
��Ĭ������£�һ���̵߳�ջҪԤ��1M���ڴ�ռ䣬��������һ�������������Կ�2048���߳�
��ʵ����ԶԶ�ﲻ����
��Ȼ��ʵ��һ��n�ķ��񣬵�����Ȼ���ʺϸ߲���������
������İ���Ҳ���Կ��������ò������ö��̣߳�����ÿ���̼߳��������󣬶�ÿ���̶߳�ռ����CPU��Դ��
��Ȼ��������в��������ǿ���ô���߳��Ƕ�cpu��Դ�ľ޴��˷ѣ�Ч�ʺܵͣ���ô�����õĽ���취��ʲô�أ�
IO��·���ã�
*/

class Server;
class Connection
{
public:
	Connection(Server* server, int clientFd) :m_server(server), m_clientFd(clientFd) {
		LOGI("");
	}
	~Connection() {
		LOGI("");

		closesocket(m_clientFd);
		// �رտͻ����ļ����������������߳���Դ


		if (th) {
			th->join();// ���� join() �������ȴ������߳�ִ����ϣ���������ǰ�߳�ֱ�������߳̽���
			delete th;
			th = nullptr;
		}
		// ͨ��ִ�����ϲ�����ȷ�������Ӷ�������ʱ����֮�����Ĺ����߳�Ҳ�ܹ�����ȷ����ֹ������������Դй©�����ҵ��߳�
	}
public:

	typedef void (*DisconnectionCallback)(void*, int);//(server, sockFd)
	/*
	����ָ������ DisconnectionCallback����ָ��һ���������ú����������������ֱ��� void* �� int ���͡�
	������˵��DisconnectionCallback ��һ������ָ�룬ָ��һ���������������ĺ�����
	1.�����ķ�������Ϊ void��
	2.������������������һ�������� void* ���ͣ��ڶ��������� int ���͡�
	�������庯��ָ�����͵ĺô��ǣ����Խ������͵ĺ���ָ����Ϊ�������ݸ����������򱣴浽�����У�������Ҫ��ʱ�������Ӧ�ĺ�����
	��������У�DisconnectionCallback ���Ա��������ӶϿ��¼��Ļص��������ͣ���������������������׽����ļ���������
	*/

	void setDisconnectionCallback(DisconnectionCallback cb, void* arg) {
		m_disconnectionCallback = cb;
		m_arg = arg;
	}
	/*
	�ú������������������ӶϿ��Ļص���������������Ӧ�Ĳ�����
	����˵����
	cb ��һ������ָ�룬ָ��һ���������ú������� DisconnectionCallback ���͵Ķ��壬
	������һ�� void* ���ͺ�һ�� int ���͵Ĳ����������� void��
	arg ��һ�� void* ���͵�ָ�룬���ڴ��ݸ��ص�������Ϊ���Ӳ�����
	����ʵ�֣�
	�� setDisconnectionCallback �����У�������Ļص�����ָ�� cb ��ֵ����Ա���� m_disconnectionCallback��
	�������Ӳ��� arg ��ֵ����Ա���� m_arg��

	ͨ������ setDisconnectionCallback ���������Խ��Զ���Ļص���������Ϊ���ӶϿ�ʱ�Ĵ�����������������ĸ��Ӳ�����
	�����������ӶϿ��¼�����ʱ������ͨ������ m_disconnectionCallback ����ָ����ִ����Ӧ�Ļص������������� m_arg ��Ϊ������
	*/

	int start() {
		th = new std::thread([](Connection* conn) {
			/*
			[] �ǲ����б�Capture List��������ָ�� Lambda ���ʽ���Ƿ���Ҫ�����ⲿ��������������η��ʡ�
			�յĲ����б� [] ��ʾ�������κ��ⲿ������Ҳ�����������ⲿ�����е��κα�����

			(Connection* conn) �ǲ����б�����ָ�� Lambda ���ʽ�Ĳ���������������У������б� (Connection* conn) ָ����һ����Ϊ conn �Ĳ���������Ϊ Connection*����ʾ����һ�� Connection ���͵�ָ����Ϊ������
	
			ͨ���������﷨�����Զ���һ�����������ⲿ������ Lambda ������������һ�� Connection* ���͵�ָ����Ϊ������
			�� Lambda �������ڲ�������ʹ�� conn ���������ʺͲ������ݽ��������Ӷ���
			*/
			int size;
			uint64_t totalSize = 0;
			time_t  t1 = time(NULL);

			while (true) {
				char buf[1024];
				memset(buf, 1, sizeof(buf));
				size = ::send(conn->m_clientFd, buf, sizeof(buf), 0);
				/* ʹ�� ::send ����ȷ�������е��õ��Ǳ�׼���е� send �������������������ܴ��ڵ�ͬ��������
				   �������Ա���Ǳ�ڵ�������ͻ���������Ϊ��

				   size_t send(int sockfd, const void *buf, size_t len, int flags);
				     sockfd����ʾҪ�������ݵ��׽����ļ���������
					 buf��ָ��Ҫ�������ݵĻ�������ָ�롣
					 len����ʾҪ���͵����ݵĳ��ȡ�
					 flags����ѡ�ķ��ͱ�־���������ڿ��Ʒ��Ͳ�������Ϊ��
				*/

				if (size < 0) {
					printf("clientFd=%d,send error�������룺%d\n", conn->m_clientFd, WSAGetLastError());

					conn->m_disconnectionCallback(conn->m_arg, conn->m_clientFd);
					break;
				}
				totalSize += size;

				if (totalSize > 62914560)/* 62914560=60*1024*1024=60mb*/
				{
					time_t t2 = time(NULL);
					if (t2 - t1 > 0) {
						uint64_t speed = totalSize / 1024 / 1024 / (t2 - t1); // �����ױ���ÿ�루Mbps��Ϊ��λ�Ĵ����ٶ�

						printf("clientFd=%d,size=%d,totalSize=%llu,speed=%lluMbps\n", conn->m_clientFd, size, totalSize, speed);

						totalSize = 0;
						t1 = time(NULL);
					}
				}
			}
			/*
			�߳��ڲ������˱��� size��totalSize �� t1������ͳ�Ʒ��͵����ݴ�С��ʱ�䡣
			��һ������ѭ���У����ȴ���һ����Ϊ buf ���ַ����飬�������ʼ��Ϊ�ض���ֵ��
			Ȼ��ʹ�� send ������ buf �е����ݷ��͸����ӵĿͻ��ˡ��������ʧ�ܣ�size < 0�������ӡ������Ϣ��
			������ conn->m_disconnectionCallback �ص������������ӵ��׽����ļ��������ʹ�������Ϊ�������ݣ����ж�ѭ����
			������ͳɹ��������͵����ݴ�С�ۼӵ� totalSize �С�
			����ۼƵ����ݴ�С���� 60MB���� 62914560 �ֽڣ�������㵱ǰ�ٶȲ���ӡ�����Ϣ��
			Ȼ�� totalSize ����Ϊ 0�������¼�ʱ�� t1 ��ֵ��
			ѭ�������ִ�У�������һ�����ݡ�
			*/
			}, this);
		return 0;
	}
	int getClientFd() {
		return m_clientFd;
	}
private:
	// Server���󣺱������������ķ���������
	Server* m_server;

	// �ͻ����ļ���������������ͻ��˽������׽����ļ�������
	int m_clientFd;

	// ָ�� std::thread �����ָ�룺�������ӵĹ����߳�
	std::thread* th = nullptr;;

	// ����ָ�룺ָ�������ӶϿ��Ļص�����
	DisconnectionCallback m_disconnectionCallback = nullptr;

	// ����ص������ĸ��Ӳ���
	void* m_arg = nullptr;// server *
};


class Server
{
public:
	Server(const char* ip, uint16_t port);
	~Server();

public:
	int start() {
		LOGI("TcpServer2 tcp://%s:%d", m_ip, m_port);
		
		m_sockFd = socket(AF_INET, SOCK_STREAM, 0);//�������������������������Ϊ���׽��ָ��߲�εĳ��󣬷��ͻ�������ݡ�������ر����Ӷ�����ͨ�����׽������������У���Ȼ������Ҫsocket������������bind��

		if (m_sockFd < 0) {
			LOGI("create socket error");
			return -1;
		}
		int on = 1;
		setsockopt(m_sockFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
		/*
		setsockopt �������������׽���ѡ������޸��׽��ֵ���Ϊ������
		setsockopt �����Ĳ����������£�
		1.m_sockFd���׽�������������ʾҪ����ѡ����׽��֡�
		2.SOL_SOCKET��ѡ��ļ��𣬱�ʾҪ���õ����׽��ּ����ѡ�
		3.SO_REUSEADDR��ѡ�����ƣ���ʾҪ���õľ���ѡ�
		4.(const char*)&on��ѡ��ֵ��ָ��洢ѡ��ֵ���ڴ��ַ���ڴ˴����� on �ĵ�ַת��Ϊ const char* ���͡�
		5.sizeof(on)��ѡ��ֵ�Ĵ�С����ʾҪ���õ�ѡ��ֵ���ֽ�����

		SO_REUSEADDR �����������������׽��ֵĵ�ַ���ù��ܡ�ͨ�����ø�ѡ��Ϊ����ֵ������������ͬһ�˿��ϰ󶨶���׽��֣���ʹ֮ǰ���׽��ֻ����� TIME_WAIT ״̬��������������·ǳ����ã�
		1.������������ֹ��������֮ǰ���׽��ֿ�����Ȼ�� TIME_WAIT ״̬��
		  ��������� SO_REUSEADDR�����޷��������°󶨵���ͬ�Ķ˿��ϣ���������������ʧ�ܡ�
		2.������̻��߳���Ҫ�󶨵���ͬ�ĵ�ַ�Ͷ˿ڣ���ʵ�ָ��ؾ���򲢷�����
		*/

		// bind
		SOCKADDR_IN server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); // ���÷�����IP
		//server_addr.sin_addr.s_addr = inet_addr("192.168.2.61");
		server_addr.sin_port = htons(m_port);

		if (bind(m_sockFd, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
			LOGI("socket bind error");
			return -1;
		}

		// listen
		if (listen(m_sockFd, 10) < 0) //listen��ָ�����׽���m_sockFd���Ϊ�����׽��֣���ʼ�����������������
		{
			LOGE("socket listen error");
			return -1;
		}

		while (true) {
			LOGI("��������������...");
			// ������������ start
			int clientFd; // �����׽���������
			char clientIp[40] = { 0 }; // �洢�ͻ��˵�IP��ַ
			uint16_t clientPort; // �洢�ͻ��˵Ķ˿ں�

			socklen_t len = 0; // ָ�����յ��ĵ�ַ�ṹ�ĳ��ȡ�
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			len = sizeof(addr);

			clientFd = accept(m_sockFd, (sockaddr*)&addr, &len);  // accept �������ڽ��ܴ�����������󣬲�����һ���µ��׽�������ͻ��˽���ͨ��
			/*
			* ��ͣ������ʽ���� 
			* 
			* ���Ѽ�����m_sockFd�׽����н���һ���ͻ������ӣ�������һ���µ��׽����ļ���������������ͻ��˽���ͨ��
			* 
			* ��һ�������Ƿ������׽����ļ������� m_sockFd
			* �ڶ�������addr���ڽ��տͻ��˵ĵ�ַ��Ϣ������ addr �� sockaddr_in ���͵Ľṹ�������
			* ��Ҫ����ǿ��ת��Ϊsockaddr*����ָ�룩
			* ����������len��һ��ָ�� socklen_t ���͵�ָ�룬����ָ�����յ��ĵ�ַ�ṹ�ĳ���
			*/
			if (clientFd < 0)
			{
				LOGE("socket accept error");
				return -1;
			}
			strcpy(clientIp, inet_ntoa(addr.sin_addr)); // ʹ�� inet_ntoa ������ addr.sin_addr �е�IP��ַת��Ϊ�ַ�������ʹ�� strcpy �������临�Ƶ� clientIp ������
			clientPort = ntohs(addr.sin_port); //  ntohs ������ addr.sin_port �еĶ˿ںŴ������ֽ�˳��ת��Ϊ�����ֽ�˳�򣬲����丳ֵ�� clientPort ����

			// ������������ end
			LOGI("���������ӣ�clientIp=%s,clientPort=%d,clientFd=%d", clientIp, clientPort, clientFd);


			Connection* conn = new Connection(this, clientFd);
			conn->setDisconnectionCallback(Server::cbDisconnection, this); // ��cbDisconnectionʵ����Connection�еĻص�������
			this->addConnection(conn);
			conn->start();//�����������߳�������

		}
		return 0;
	}
	void handleDisconnection(int clientFd) {
		LOGI("clientFd=%d", clientFd);
		this->removeConnection(clientFd);
	}
	static void cbDisconnection(void* arg, int clientFd) {
		LOGI("clientFd=%d", clientFd);
		Server* server = (Server*)arg;
		server->handleDisconnection(clientFd);
	}
private:
	const char* m_ip; // ������ip
	uint16_t m_port; // �������˿�
	int m_sockFd; // �������׽���������

	std::map<int, Connection*> m_connMap;// <sockFd,conn> accept���������Ӧ������  ά�����б����������� 
	std::mutex m_connMap_mtx; // ���������̰߳�ȫ�Ĳ�����ȷ������̲߳���ͬʱ�޸�����


	bool addConnection(Connection* conn) {
		m_connMap_mtx.lock();
		if (m_connMap.find(conn->getClientFd()) != m_connMap.end()) {
			m_connMap_mtx.unlock();
			return false;
		}
		else {
			m_connMap.insert(std::make_pair(conn->getClientFd(), conn));
			m_connMap_mtx.unlock();
			return true;
		}
	}


	Connection* getConnection(int clientFd) {
		m_connMap_mtx.lock();
		std::map<int, Connection*>::iterator it = m_connMap.find(clientFd);
		if (it != m_connMap.end()) {
			m_connMap_mtx.unlock();
			return it->second;
		}
		else {
			m_connMap_mtx.unlock();
			return nullptr;
		}
	}


	bool removeConnection(int clientFd) {
		m_connMap_mtx.lock();
		std::map<int, Connection*>::iterator it = m_connMap.find(clientFd);
		if (it != m_connMap.end()) {
			m_connMap.erase(it);
			m_connMap_mtx.unlock();
			return true;
		}
		else {
			m_connMap_mtx.unlock();
			return false;
		}
	}
};


Server::Server(const char* ip, uint16_t port) :m_ip(ip), m_port(port), m_sockFd(-1) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		LOGE("WSAStartup Error");
		return;
	}
}
/*
������� `ip` �� `port` �����ֱ�ֵ����Ա���� `m_ip` �� `m_port`
ͬʱ��`m_sockFd` ��Ա������ʼ��Ϊ -1��
��������ʹ�� Winsock API �е� `WSAStartup` ��������ʼ�� Winsock �⡣`WSADATA` �ṹ�� `wsaData` ���ڱ����ʼ���������Ϣ��
ͨ������ `WSAStartup(MAKEWORD(2, 2), &wsaData)`������ָ��ʹ�õ� Winsock �汾������ʹ�õ��� 2.2 �汾��
��� `WSAStartup` ���ص�ֵ��Ϊ 0����ʾ��ʼ��ʧ�ܡ�����������£����ӡ������Ϣ `"WSAStartup Error"`��Ȼ�󷵻ع��캯����
����Ĵ������̱��жϡ�
�������캯����Ŀ���ǳ�ʼ������������׼��ʹ�� Winsock ���������ͨ�š������ʼ��ʧ�ܣ��Ͳ��ᴴ����Ч�ķ���������
*/

Server::~Server() {
	if (m_sockFd > -1) {
		closesocket(m_sockFd);
		m_sockFd = -1;
	}
	WSACleanup();
}


int main() {
	const char* ip = "127.0.0.1";
	uint16_t port = 8080;

	Server server(ip, port);
	server.start();

	return 0;
}