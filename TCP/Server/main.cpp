/*
 * C++ server demo
 * @author:drizzledrop
 * @time:2023.3.19
 * */

#include <iostream>
#include <Windows.h>
/*
 * #pragma comment(lib,"ws2_32.lib") 是一个在Windows平台上的编译指令（compiler directive）
 * 用于告诉编译器将 ws2_32.lib 这个库文件链接到生成的可执行文件中。
 * 这个库文件是Windows SocketServer 2.0 API的动态链接库，它包含了一系列的函数和数据类型，
 * 可以被用于实现基于网络的应用程序。这个指令通常在编写Windows平台下的网络编程代码时使用。*/
#pragma comment(lib,"ws2_32.lib")

int main() {

    printf("\r\nSocketServer start\r\n");
    /*
     * WSADATA 是Windows Sockets API中的一个结构体，
     * 用于存储Winsock库的详细信息，包括版本、实现和支持的协议等。
     * 在调用 WSAStartup 函数时，必须提供一个指向 WSADATA 结构体的指针，
     * 以便接收Winsock库的详细信息。在实际编程中，通常使用类似 WSADATA wsaData;
     * 或者 WSADATA wsaData = {}; 的方式定义 wsaData 变量，
     * 并将其作为参数传递给 WSAStartup 函数。
     * */
    WSADATA wsaData{};


    SOCKET SerSock{}, CliSock{}; // serSock是欢迎套接字 CliSock是后续连接的套接字
    sockaddr_in SerAddr{}, CliAddr{};
    /*
     * WSAStartup 是Windows Sockets API中的一个函数，
     * 用于初始化Winsock库，它必须在应用程序中使用Windows SocketServer API之前被调用。
     * WSA是Windows SocketServer API的缩写。
     * 具体来说，当一个应用程序使用Windows SocketServer API时，
     * 它必须首先调用 WSAStartup 函数来初始化Winsock库，以便进行网络通信。
     *      * 参数说明如下：
     * int WSAStartup(
     *      WORD      wVersionRequested,
     *      LPWSADATA lpWSAData);
     *
     * wVersionRequested表示需要使用的Winsock版本号，通常设置为MAKEWORD(2, 1)表示使用Winsock2.1版本
     *      */

    /*
     * WSAGetLastError() 是Windows Sockets API中的一个函数，
     * 用于获取最近一次发生的套接字错误代码（SocketServer Error）。
     * 在使用Windows SocketServer API进行网络编程时，可能会遇到各种错误情况，
     * 例如连接失败、发送数据失败等等。这些错误情况都会导致一个Socket Error，
     * 即套接字错误。这个错误代码通常是一个整数值，表示错误的类型和具体原因。
     * 可以通过 WSAGetLastError() 函数获取这个错误代码，进而对错误情况进行处理。*/
    if(WSAStartup(MAKEWORD(2,1), &wsaData) == -1){
        printf("Error(WSAStartup) ErrorNumber(%d)\r\n",WSAGetLastError());
        return -1;
    }

    /*
     * 参数 PF_INET, SOCK_STREAM, 0 分别分别代表了Socket的地址族、Socket的类型和Socket所使用的协议。
     * PF_INET表示使用IPv4地址族，也就是使用IPv4协议进行通信。(在Windows平台上，AF_INET与PF_INET是等价的，都表示IPv4协议族)
     * SOCK_STREAM表示创建一个面向连接的TCP协议的Socket，它提供了可靠的、基于流的、全双工的通信机制，适用于一对一的客户端和服务器通信。
     * 0表示根据第二个参数自动选择使用哪个协议，这里的第二个参数是SOCK_STREAM，所以会自动选择TCP协议。
     * 综上，这个函数的意思是创建一个使用TCP协议、IPv4地址族的Socket，并返回该Socket的文件描述符。*/
    /*
     * WSACleanup() 是一个 Windows Sockets API 函数，用于关闭 Winsock 动态链接库 (DLL) 使用的资源，
     * 包括释放资源、套接字句柄等。在使用完 Winsock 后，调用此函数可以确保资源得到正确的释放，避免内存泄漏和资源占用。
     * 在使用 Winsock 应用程序时，通常在程序结束前调用 WSACleanup() 函数以清理资源。
     * 需要注意的是，一旦调用了 WSACleanup() 函数，就不能再使用 Winsock 动态链接库中的任何其他函数。
     * 如果需要在后续再次使用 Winsock，必须重新调用 WSAStartup() 函数。
     * */
    if((SerSock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        printf("socket ErrorNumber(%d)\r\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    SerAddr.sin_family = AF_INET;
//    SerAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 回环地址
    /*
     * sin_addr是in_addr类型结构体，用于表示IPv4地址的结构体，
     * 其中s_addr字段是一个32位整数，表示IPv4地址的二进制形式。
     * struct in_addr {
     *  unsigned long s_addr;
     * };
     * INADDR_ANY是一个宏定义，表示绑定所有网卡的IPv4地址，也即是0.0.0.0
     * htonl()函数是一个字节序转换函数，用于将一个32位整数从主机字节序（即本地字节序,一般为小端）
     * 转换为网络字节序（即大端字节序）
     * 在将IP地址和端口号转换为网络字节序后，
     * 可以通过调用bind()函数将socket绑定到相应的地址和端口
     * */
    SerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    SerAddr.sin_port = htons(10003);

    /*
     * struct sockaddr_in {
     *      short int sin_family;           // 地址族，如 AF_INET
     *      unsigned short int sin_port;    // 端口号
     *      struct in_addr sin_addr;        // IPv4 地址结构体
     *      unsigned char sin_zero[8];      // 保留字节，一般全部设为0
     * };
     *
     *
     * struct sockaddr {
     *      unsigned short sa_family;    // 地址族，如 AF_INET
     *      char sa_data[14];           // 地址信息
     * };
     * */
    /*
     *在C++中，bind()函数用于将一个 socket 绑定到一个特定的 IP 地址和端口上。其函数原型为：
     * int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
     * 参数说明如下：
     * sockfd：需要绑定的 socket 描述符；
     * addr：指向要绑定的目标地址的 sockaddr 结构体指针，可以是 sockaddr_in、sockaddr_in6 或其他类型；
     * addrlen：目标地址的长度，以字节为单位。通常使用 sizeof() 来获取地址结构体的长度。
     * 其中，参数 addr 指向的结构体类型（如 sockaddr_in）取决于所使用的协议（如 IPv4 或 IPv6）。
     * 在 bind() 函数成功执行后，该 socket 就被绑定到了指定的 IP 地址和端口上，可以开始监听连接请求或者与其他 socket 建立连接。如果函数执行失败，则会返回 -1，此时可以使用 perror() 函数打印错误信息。
     * */
    // 给服务器socket绑定 地址信息结构体
    // 绑定地址
    if(bind(SerSock, (SOCKADDR*)&SerAddr, sizeof(SerAddr)) == -1){
        printf("bind ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(SerSock);
        WSACleanup();
        return -1;
    }

    // 等待队列
    if(listen(SerSock, 5) == -1){
        printf("listen ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(SerSock);
        WSACleanup();
        return -1;
    }

    int len = sizeof(CliAddr);
    // 等待连接 accept是阻塞函数
    /*
     * SOCKET accept(SOCKET s,sockaddr *addr,int *addrlen);
     * 参数说明如下：
     * s：类型为 SOCKET，表示服务器端的套接字，即调用 listen() 函数创建的套接字。
     * addr：类型为 sockaddr*，表示一个指向 sockaddr 结构体的指针，该结构体包含客户端的地址信息。
     * addrlen：类型为 int*，表示一个指向整数的指针，该整数指定 addr 结构体的长度。
     *
     * accept() 函数的作用是在服务器端等待并接受客户端的连接请求，并返回一个新的套接字，
     * 该套接字用于与客户端进行通信。addr 和 addrlen 两个参数用于返回客户端的地址信息，
     * 可以通过这些信息与客户端进行通信。在函数调用成功时，返回值为一个新的套接字，
     * 用于与客户端进行通信。如果函数调用失败，则返回值为 INVALID_SOCKET，错误码可以通过 WSAGetLastError() 函数获取。
     * */
    if((CliSock = accept(SerSock, (SOCKADDR*)&CliAddr, &len)) == -1){
        printf("accept ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(SerSock);
        WSACleanup();
        return -1;
    }
    printf("Connect to Client\n");

    /*
     * recv 接受它发送的数据
     * int recv(int sockfd, void *buf, size_t len, int flags);
     * 其中各参数含义如下：
     * sockfd：需要接收数据的socket的描述符（file descriptor）
     * buf：指向接收数据的缓冲区
     * len：缓冲区长度，即最大接收数据的长度
     * flags：接收数据的方式，常用取值为0，表示阻塞方式，即在接收到数据之前会一直等待
     * 该函数返回值为实际接收到的数据长度，如果返回-1则表示接收数据失败。
     * */
#define BUF_SIZE 0X100
    char RecvBuf[BUF_SIZE]{};
    int ret = recv(CliSock, RecvBuf, sizeof(RecvBuf), 0);
    if(ret <= 0){
        printf("recv ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(SerSock);
        WSACleanup();
        return -1;
    }

    printf("RecvBuf: %s\r\n", RecvBuf);

    /*
     * send 发送字节流数据
     *C++中的send函数用于向指定的socket发送数据。它的函数声明如下：
     * int send(int socket, const void* buffer, size_t length, int flags);
     * 其中，参数含义如下：
     * socket：需要发送数据的socket文件描述符；
     * buffer：指向存储数据的缓冲区的指针；
     * length：需要发送的数据的字节数；
     * flags：发送数据时使用的标志。
     * 函数返回值为发送的字节数，或者在发生错误时返回SOCKET_ERROR。
     * */
    char SendBuf[]{"Server, Hello, World!"};
    ret = send(CliSock, SendBuf, sizeof(SendBuf), 0);
    if(ret <= 0){
        printf("send ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(SerSock);
        WSACleanup();
        return -1;
    }
    printf("\r\nSocketServer end\r\n");
    closesocket(CliSock);
    closesocket(SerSock);
    WSACleanup();
    return 0;
}
