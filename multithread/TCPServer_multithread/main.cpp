#include <stdint.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

//  __FILE__ 获取源文件的相对路径和名字
//  __LINE__ 获取该行代码在文件中的行号
//  __func__ 或 __FUNCTION__ 获取函数名

#define LOGI(format, ...)  fprintf(stderr,"[INFO] [%s:%d]:%s() " format "\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOGE(format, ...)  fprintf(stderr,"[ERROR] [%s:%d]:%s() " format "\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)
/*
定义了宏(LOGI)，用于方便地输出带有信息级别、文件名、行号、函数名以及自定义格式的日志消息。
*/

/*
无论是linux还是windows，一个进程可用的内存空间都有上限，比如windows内存空间上限有2G，
而默认情况下，一个线程的栈要预留1M的内存空间，所以理论一个进程中最多可以开2048个线程
但实际上远远达不到。
虽然能实现一对n的服务，但很显然不适合高并发场景。
从下面的案例也可以看出来，该并发利用多线程，但是每个线程计算量不大，而每个线程都占用了CPU资源。
虽然我们想进行并发，但是开这么多线程是对cpu资源的巨大浪费，效率很低！那么，更好的解决办法是什么呢？
IO多路复用！
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
		// 关闭客户端文件描述符，并清理线程资源


		if (th) {
			th->join();// 调用 join() 函数，等待工作线程执行完毕，即阻塞当前线程直到工作线程结束
			delete th;
			th = nullptr;
		}
		// 通过执行以上操作，确保在连接对象被销毁时，与之关联的工作线程也能够被正确地终止和清理，避免资源泄漏和悬挂的线程
	}
public:

	typedef void (*DisconnectionCallback)(void*, int);//(server, sockFd)
	/*
	函数指针类型 DisconnectionCallback，它指向一个函数，该函数有两个参数，分别是 void* 和 int 类型。
	具体来说，DisconnectionCallback 是一个函数指针，指向一个具有以下特征的函数：
	1.函数的返回类型为 void。
	2.函数有两个参数，第一个参数是 void* 类型，第二个参数是 int 类型。
	这样定义函数指针类型的好处是，可以将该类型的函数指针作为参数传递给其他函数或保存到变量中，并在需要的时候调用相应的函数。
	这个例子中，DisconnectionCallback 可以被用作连接断开事件的回调函数类型，参数包括服务器对象和套接字文件描述符。
	*/

	void setDisconnectionCallback(DisconnectionCallback cb, void* arg) {
		m_disconnectionCallback = cb;
		m_arg = arg;
	}
	/*
	该函数的作用是设置连接断开的回调函数，并传入相应的参数。
	参数说明：
	cb 是一个函数指针，指向一个函数，该函数符合 DisconnectionCallback 类型的定义，
	即接受一个 void* 类型和一个 int 类型的参数，并返回 void。
	arg 是一个 void* 类型的指针，用于传递给回调函数作为附加参数。
	函数实现：
	在 setDisconnectionCallback 函数中，将传入的回调函数指针 cb 赋值给成员变量 m_disconnectionCallback，
	并将附加参数 arg 赋值给成员变量 m_arg。

	通过调用 setDisconnectionCallback 函数，可以将自定义的回调函数设置为连接断开时的处理函数，并传递所需的附加参数。
	这样，在连接断开事件发生时，可以通过调用 m_disconnectionCallback 函数指针来执行相应的回调函数，并传递 m_arg 作为参数。
	*/

	int start() {
		th = new std::thread([](Connection* conn) {
			/*
			[] 是捕获列表（Capture List），用于指定 Lambda 表达式中是否需要访问外部变量，并控制如何访问。
			空的捕获列表 [] 表示不捕获任何外部变量，也即不依赖于外部环境中的任何变量。

			(Connection* conn) 是参数列表，用于指定 Lambda 表达式的参数。在这个例子中，参数列表 (Connection* conn) 指定了一个名为 conn 的参数，类型为 Connection*，表示接收一个 Connection 类型的指针作为参数。
	
			通过这样的语法，可以定义一个不依赖于外部环境的 Lambda 函数，它接受一个 Connection* 类型的指针作为参数。
			在 Lambda 函数体内部，可以使用 conn 参数来访问和操作传递进来的连接对象。
			*/
			int size;
			uint64_t totalSize = 0;
			time_t  t1 = time(NULL);

			while (true) {
				char buf[1024];
				memset(buf, 1, sizeof(buf));
				size = ::send(conn->m_clientFd, buf, sizeof(buf), 0);
				/* 使用 ::send 可以确保代码中调用的是标准库中的 send 函数，而不是其他可能存在的同名函数。
				   这样可以避免潜在的命名冲突和意外的行为。

				   size_t send(int sockfd, const void *buf, size_t len, int flags);
				     sockfd：表示要发送数据的套接字文件描述符。
					 buf：指向要发送数据的缓冲区的指针。
					 len：表示要发送的数据的长度。
					 flags：可选的发送标志，可以用于控制发送操作的行为。
				*/

				if (size < 0) {
					printf("clientFd=%d,send error，错误码：%d\n", conn->m_clientFd, WSAGetLastError());

					conn->m_disconnectionCallback(conn->m_arg, conn->m_clientFd);
					break;
				}
				totalSize += size;

				if (totalSize > 62914560)/* 62914560=60*1024*1024=60mb*/
				{
					time_t t2 = time(NULL);
					if (t2 - t1 > 0) {
						uint64_t speed = totalSize / 1024 / 1024 / (t2 - t1); // 计算兆比特每秒（Mbps）为单位的传输速度

						printf("clientFd=%d,size=%d,totalSize=%llu,speed=%lluMbps\n", conn->m_clientFd, size, totalSize, speed);

						totalSize = 0;
						t1 = time(NULL);
					}
				}
			}
			/*
			线程内部定义了变量 size、totalSize 和 t1，用于统计发送的数据大小和时间。
			在一个无限循环中，首先创建一个名为 buf 的字符数组，并将其初始化为特定的值。
			然后使用 send 函数将 buf 中的数据发送给连接的客户端。如果发送失败（size < 0），则打印出错信息，
			并调用 conn->m_disconnectionCallback 回调函数，将连接的套接字文件描述符和错误码作为参数传递，并中断循环。
			如果发送成功，将发送的数据大小累加到 totalSize 中。
			如果累计的数据大小超过 60MB（即 62914560 字节），则计算当前速度并打印相关信息。
			然后将 totalSize 重置为 0，并更新计时器 t1 的值。
			循环会继续执行，发送下一批数据。
			*/
			}, this);
		return 0;
	}
	int getClientFd() {
		return m_clientFd;
	}
private:
	// Server对象：保存连接所属的服务器对象
	Server* m_server;

	// 客户端文件描述符：保存与客户端建立的套接字文件描述符
	int m_clientFd;

	// 指向 std::thread 对象的指针：保存连接的工作线程
	std::thread* th = nullptr;;

	// 函数指针：指向处理连接断开的回调函数
	DisconnectionCallback m_disconnectionCallback = nullptr;

	// 保存回调函数的附加参数
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
		
		m_sockFd = socket(AF_INET, SOCK_STREAM, 0);//创建描述符，描述符可以理解为对套接字更高层次的抽象，发送或接受数据、建立或关闭链接都可以通过该套接字描述符进行（当然，这需要socket和描述符进行bind）

		if (m_sockFd < 0) {
			LOGI("create socket error");
			return -1;
		}
		int on = 1;
		setsockopt(m_sockFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
		/*
		setsockopt 函数用于设置套接字选项，可以修改套接字的行为和属性
		setsockopt 函数的参数解释如下：
		1.m_sockFd：套接字描述符，表示要设置选项的套接字。
		2.SOL_SOCKET：选项的级别，表示要设置的是套接字级别的选项。
		3.SO_REUSEADDR：选项名称，表示要设置的具体选项。
		4.(const char*)&on：选项值，指向存储选项值的内存地址。在此处，将 on 的地址转换为 const char* 类型。
		5.sizeof(on)：选项值的大小，表示要设置的选项值的字节数。

		SO_REUSEADDR 在这里是用于设置套接字的地址重用功能。通过设置该选项为非零值，可以允许在同一端口上绑定多个套接字，即使之前的套接字还处于 TIME_WAIT 状态。这在以下情况下非常有用：
		1.服务器意外终止或重启后，之前的套接字可能仍然在 TIME_WAIT 状态，
		  如果不设置 SO_REUSEADDR，将无法立即重新绑定到相同的端口上，导致启动服务器失败。
		2.多个进程或线程需要绑定到相同的地址和端口，以实现负载均衡或并发处理。
		*/

		// bind
		SOCKADDR_IN server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); // 设置服务器IP
		//server_addr.sin_addr.s_addr = inet_addr("192.168.2.61");
		server_addr.sin_port = htons(m_port);

		if (bind(m_sockFd, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
			LOGI("socket bind error");
			return -1;
		}

		// listen
		if (listen(m_sockFd, 10) < 0) //listen将指定的套接字m_sockFd标记为被动套接字，开始监听传入的连接请求
		{
			LOGE("socket listen error");
			return -1;
		}

		while (true) {
			LOGI("阻塞监听新连接...");
			// 阻塞接收请求 start
			int clientFd; // 链接套接字描述符
			char clientIp[40] = { 0 }; // 存储客户端的IP地址
			uint16_t clientPort; // 存储客户端的端口号

			socklen_t len = 0; // 指定接收到的地址结构的长度。
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			len = sizeof(addr);

			clientFd = accept(m_sockFd, (sockaddr*)&addr, &len);  // accept 函数用于接受传入的连接请求，并创建一个新的套接字来与客户端进行通信
			/*
			* 不停地阻塞式监听 
			* 
			* 从已监听的m_sockFd套接字中接受一个客户端连接，并返回一个新的套接字文件描述符，用于与客户端进行通信
			* 
			* 第一个参数是服务器套接字文件描述符 m_sockFd
			* 第二个参数addr用于接收客户端的地址信息（由于 addr 是 sockaddr_in 类型的结构体变量，
			* 需要将其强制转换为sockaddr*类型指针）
			* 第三个参数len是一个指向 socklen_t 类型的指针，用于指定接收到的地址结构的长度
			*/
			if (clientFd < 0)
			{
				LOGE("socket accept error");
				return -1;
			}
			strcpy(clientIp, inet_ntoa(addr.sin_addr)); // 使用 inet_ntoa 函数将 addr.sin_addr 中的IP地址转换为字符串，并使用 strcpy 函数将其复制到 clientIp 数组中
			clientPort = ntohs(addr.sin_port); //  ntohs 函数将 addr.sin_port 中的端口号从网络字节顺序转换为主机字节顺序，并将其赋值给 clientPort 变量

			// 阻塞接收请求 end
			LOGI("发现新连接：clientIp=%s,clientPort=%d,clientFd=%d", clientIp, clientPort, clientFd);


			Connection* conn = new Connection(this, clientFd);
			conn->setDisconnectionCallback(Server::cbDisconnection, this); // 用cbDisconnection实例化Connection中的回调函数？
			this->addConnection(conn);
			conn->start();//非阻塞在子线程中启动

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
	const char* m_ip; // 服务器ip
	uint16_t m_port; // 服务器端口
	int m_sockFd; // 服务器套接字描述符

	std::map<int, Connection*> m_connMap;// <sockFd,conn> accept描述符与对应的连接  维护所有被创建的连接 
	std::mutex m_connMap_mtx; // 容器进行线程安全的操作，确保多个线程不会同时修改容器


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
将传入的 `ip` 和 `port` 参数分别赋值给成员变量 `m_ip` 和 `m_port`
同时，`m_sockFd` 成员变量初始化为 -1。
接下来，使用 Winsock API 中的 `WSAStartup` 函数来初始化 Winsock 库。`WSADATA` 结构体 `wsaData` 用于保存初始化结果的信息。
通过调用 `WSAStartup(MAKEWORD(2, 2), &wsaData)`，可以指定使用的 Winsock 版本，这里使用的是 2.2 版本。
如果 `WSAStartup` 返回的值不为 0，表示初始化失败。在这种情况下，会打印错误信息 `"WSAStartup Error"`，然后返回构造函数，
对象的创建过程被中断。
整个构造函数的目的是初始化服务器对象并准备使用 Winsock 库进行网络通信。如果初始化失败，就不会创建有效的服务器对象。
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