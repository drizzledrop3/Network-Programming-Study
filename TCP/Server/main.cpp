/*
 * C++ server demo
 * @author:drizzledrop
 * @time:2023.3.19
 * */

#include <iostream>
#include <Windows.h>
/*
 * #pragma comment(lib,"ws2_32.lib") ��һ����Windowsƽ̨�ϵı���ָ�compiler directive��
 * ���ڸ��߱������� ws2_32.lib ������ļ����ӵ����ɵĿ�ִ���ļ��С�
 * ������ļ���Windows SocketServer 2.0 API�Ķ�̬���ӿ⣬��������һϵ�еĺ������������ͣ�
 * ���Ա�����ʵ�ֻ��������Ӧ�ó������ָ��ͨ���ڱ�дWindowsƽ̨�µ������̴���ʱʹ�á�*/
#pragma comment(lib,"ws2_32.lib")

int main() {

    printf("\r\nSocketServer start\r\n");
    /*
     * WSADATA ��Windows Sockets API�е�һ���ṹ�壬
     * ���ڴ洢Winsock�����ϸ��Ϣ�������汾��ʵ�ֺ�֧�ֵ�Э��ȡ�
     * �ڵ��� WSAStartup ����ʱ�������ṩһ��ָ�� WSADATA �ṹ���ָ�룬
     * �Ա����Winsock�����ϸ��Ϣ����ʵ�ʱ���У�ͨ��ʹ������ WSADATA wsaData;
     * ���� WSADATA wsaData = {}; �ķ�ʽ���� wsaData ������
     * ��������Ϊ�������ݸ� WSAStartup ������
     * */
    WSADATA wsaData{};


    SOCKET SerSock{}, CliSock{}; // serSock�ǻ�ӭ�׽��� CliSock�Ǻ������ӵ��׽���
    sockaddr_in SerAddr{}, CliAddr{};
    /*
     * WSAStartup ��Windows Sockets API�е�һ��������
     * ���ڳ�ʼ��Winsock�⣬��������Ӧ�ó�����ʹ��Windows SocketServer API֮ǰ�����á�
     * WSA��Windows SocketServer API����д��
     * ������˵����һ��Ӧ�ó���ʹ��Windows SocketServer APIʱ��
     * ���������ȵ��� WSAStartup ��������ʼ��Winsock�⣬�Ա��������ͨ�š�
     *      * ����˵�����£�
     * int WSAStartup(
     *      WORD      wVersionRequested,
     *      LPWSADATA lpWSAData);
     *
     * wVersionRequested��ʾ��Ҫʹ�õ�Winsock�汾�ţ�ͨ������ΪMAKEWORD(2, 1)��ʾʹ��Winsock2.1�汾
     *      */

    /*
     * WSAGetLastError() ��Windows Sockets API�е�һ��������
     * ���ڻ�ȡ���һ�η������׽��ִ�����루SocketServer Error����
     * ��ʹ��Windows SocketServer API����������ʱ�����ܻ��������ִ��������
     * ��������ʧ�ܡ���������ʧ�ܵȵȡ���Щ����������ᵼ��һ��Socket Error��
     * ���׽��ִ�������������ͨ����һ������ֵ����ʾ��������ͺ;���ԭ��
     * ����ͨ�� WSAGetLastError() ������ȡ���������룬�����Դ���������д���*/
    if(WSAStartup(MAKEWORD(2,1), &wsaData) == -1){
        printf("Error(WSAStartup) ErrorNumber(%d)\r\n",WSAGetLastError());
        return -1;
    }

    /*
     * ���� PF_INET, SOCK_STREAM, 0 �ֱ�ֱ������Socket�ĵ�ַ�塢Socket�����ͺ�Socket��ʹ�õ�Э�顣
     * PF_INET��ʾʹ��IPv4��ַ�壬Ҳ����ʹ��IPv4Э�����ͨ�š�(��Windowsƽ̨�ϣ�AF_INET��PF_INET�ǵȼ۵ģ�����ʾIPv4Э����)
     * SOCK_STREAM��ʾ����һ���������ӵ�TCPЭ���Socket�����ṩ�˿ɿ��ġ��������ġ�ȫ˫����ͨ�Ż��ƣ�������һ��һ�Ŀͻ��˺ͷ�����ͨ�š�
     * 0��ʾ���ݵڶ��������Զ�ѡ��ʹ���ĸ�Э�飬����ĵڶ���������SOCK_STREAM�����Ի��Զ�ѡ��TCPЭ�顣
     * ���ϣ������������˼�Ǵ���һ��ʹ��TCPЭ�顢IPv4��ַ���Socket�������ظ�Socket���ļ���������*/
    /*
     * WSACleanup() ��һ�� Windows Sockets API ���������ڹر� Winsock ��̬���ӿ� (DLL) ʹ�õ���Դ��
     * �����ͷ���Դ���׽��־���ȡ���ʹ���� Winsock �󣬵��ô˺�������ȷ����Դ�õ���ȷ���ͷţ������ڴ�й©����Դռ�á�
     * ��ʹ�� Winsock Ӧ�ó���ʱ��ͨ���ڳ������ǰ���� WSACleanup() ������������Դ��
     * ��Ҫע����ǣ�һ�������� WSACleanup() �������Ͳ�����ʹ�� Winsock ��̬���ӿ��е��κ�����������
     * �����Ҫ�ں����ٴ�ʹ�� Winsock���������µ��� WSAStartup() ������
     * */
    if((SerSock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        printf("socket ErrorNumber(%d)\r\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    SerAddr.sin_family = AF_INET;
//    SerAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); �ػ���ַ
    /*
     * sin_addr��in_addr���ͽṹ�壬���ڱ�ʾIPv4��ַ�Ľṹ�壬
     * ����s_addr�ֶ���һ��32λ��������ʾIPv4��ַ�Ķ�������ʽ��
     * struct in_addr {
     *  unsigned long s_addr;
     * };
     * INADDR_ANY��һ���궨�壬��ʾ������������IPv4��ַ��Ҳ����0.0.0.0
     * htonl()������һ���ֽ���ת�����������ڽ�һ��32λ�����������ֽ��򣨼������ֽ���,һ��ΪС�ˣ�
     * ת��Ϊ�����ֽ��򣨼�����ֽ���
     * �ڽ�IP��ַ�Ͷ˿ں�ת��Ϊ�����ֽ����
     * ����ͨ������bind()������socket�󶨵���Ӧ�ĵ�ַ�Ͷ˿�
     * */
    SerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    SerAddr.sin_port = htons(10003);

    /*
     * struct sockaddr_in {
     *      short int sin_family;           // ��ַ�壬�� AF_INET
     *      unsigned short int sin_port;    // �˿ں�
     *      struct in_addr sin_addr;        // IPv4 ��ַ�ṹ��
     *      unsigned char sin_zero[8];      // �����ֽڣ�һ��ȫ����Ϊ0
     * };
     *
     *
     * struct sockaddr {
     *      unsigned short sa_family;    // ��ַ�壬�� AF_INET
     *      char sa_data[14];           // ��ַ��Ϣ
     * };
     * */
    /*
     *��C++�У�bind()�������ڽ�һ�� socket �󶨵�һ���ض��� IP ��ַ�Ͷ˿��ϡ��亯��ԭ��Ϊ��
     * int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
     * ����˵�����£�
     * sockfd����Ҫ�󶨵� socket ��������
     * addr��ָ��Ҫ�󶨵�Ŀ���ַ�� sockaddr �ṹ��ָ�룬������ sockaddr_in��sockaddr_in6 ���������ͣ�
     * addrlen��Ŀ���ַ�ĳ��ȣ����ֽ�Ϊ��λ��ͨ��ʹ�� sizeof() ����ȡ��ַ�ṹ��ĳ��ȡ�
     * ���У����� addr ָ��Ľṹ�����ͣ��� sockaddr_in��ȡ������ʹ�õ�Э�飨�� IPv4 �� IPv6����
     * �� bind() �����ɹ�ִ�к󣬸� socket �ͱ��󶨵���ָ���� IP ��ַ�Ͷ˿��ϣ����Կ�ʼ��������������������� socket �������ӡ��������ִ��ʧ�ܣ���᷵�� -1����ʱ����ʹ�� perror() ������ӡ������Ϣ��
     * */
    // ��������socket�� ��ַ��Ϣ�ṹ��
    // �󶨵�ַ
    if(bind(SerSock, (SOCKADDR*)&SerAddr, sizeof(SerAddr)) == -1){
        printf("bind ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(SerSock);
        WSACleanup();
        return -1;
    }

    // �ȴ�����
    if(listen(SerSock, 5) == -1){
        printf("listen ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(SerSock);
        WSACleanup();
        return -1;
    }

    int len = sizeof(CliAddr);
    // �ȴ����� accept����������
    /*
     * SOCKET accept(SOCKET s,sockaddr *addr,int *addrlen);
     * ����˵�����£�
     * s������Ϊ SOCKET����ʾ�������˵��׽��֣������� listen() �����������׽��֡�
     * addr������Ϊ sockaddr*����ʾһ��ָ�� sockaddr �ṹ���ָ�룬�ýṹ������ͻ��˵ĵ�ַ��Ϣ��
     * addrlen������Ϊ int*����ʾһ��ָ��������ָ�룬������ָ�� addr �ṹ��ĳ��ȡ�
     *
     * accept() �������������ڷ������˵ȴ������ܿͻ��˵��������󣬲�����һ���µ��׽��֣�
     * ���׽���������ͻ��˽���ͨ�š�addr �� addrlen �����������ڷ��ؿͻ��˵ĵ�ַ��Ϣ��
     * ����ͨ����Щ��Ϣ��ͻ��˽���ͨ�š��ں������óɹ�ʱ������ֵΪһ���µ��׽��֣�
     * ������ͻ��˽���ͨ�š������������ʧ�ܣ��򷵻�ֵΪ INVALID_SOCKET�����������ͨ�� WSAGetLastError() ������ȡ��
     * */
    if((CliSock = accept(SerSock, (SOCKADDR*)&CliAddr, &len)) == -1){
        printf("accept ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(SerSock);
        WSACleanup();
        return -1;
    }
    printf("Connect to Client\n");

    /*
     * recv ���������͵�����
     * int recv(int sockfd, void *buf, size_t len, int flags);
     * ���и������������£�
     * sockfd����Ҫ�������ݵ�socket����������file descriptor��
     * buf��ָ��������ݵĻ�����
     * len�����������ȣ������������ݵĳ���
     * flags���������ݵķ�ʽ������ȡֵΪ0����ʾ������ʽ�����ڽ��յ�����֮ǰ��һֱ�ȴ�
     * �ú�������ֵΪʵ�ʽ��յ������ݳ��ȣ��������-1���ʾ��������ʧ�ܡ�
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
     * send �����ֽ�������
     *C++�е�send����������ָ����socket�������ݡ����ĺ����������£�
     * int send(int socket, const void* buffer, size_t length, int flags);
     * ���У������������£�
     * socket����Ҫ�������ݵ�socket�ļ���������
     * buffer��ָ��洢���ݵĻ�������ָ�룻
     * length����Ҫ���͵����ݵ��ֽ�����
     * flags����������ʱʹ�õı�־��
     * ��������ֵΪ���͵��ֽ����������ڷ�������ʱ����SOCKET_ERROR��
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
