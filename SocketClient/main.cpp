/*
 * C++ client demo
 * @author: drizzledrop
 * @time:2023.3.19
 * */

#include <iostream>
#include <Windows.h>
#pragma comment(lib,"ws2_32.lib")

int main() {
    printf("\r\nSocketClient start\r\n");
    WSADATA wsaData{};
    SOCKET CliSock{};
    sockaddr_in CliAddr{};

    if (WSAStartup(MAKEWORD(2, 1), &wsaData) == -1) {
        printf("Error(WSAStartup) ErrorNumber(%d)\r\n", WSAGetLastError());
        return -1;
    }

    if ((CliSock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        printf("socket ErrorNumber(%d)\r\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    CliAddr.sin_family = AF_INET;
    CliAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 此处为server的IP地址
    CliAddr.sin_port = htons(10003); // 此处为server的端口号

    if (connect(CliSock, (SOCKADDR*)&CliAddr, sizeof(SOCKADDR)) == -1) {
        printf("connect ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(CliSock);
        WSACleanup();
        return -1;
    }

    printf("Connect to Server\n");

    char SendBuf[]{"Client, Hello, World!"};
    int ret = send(CliSock, SendBuf, sizeof(SendBuf), 0);
    if(ret <= 0){
        printf("send ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(CliSock);
        WSACleanup();
        return -1;
    }

#define BUF_SIZE 0X100
    char RecvBuf[BUF_SIZE]{};
    ret = recv(CliSock, RecvBuf, sizeof(RecvBuf), 0);
    if(ret <= 0){
        printf("recv ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(CliSock);
        WSACleanup();
        return -1;
    }
    printf("RecvBuf: %s\r\n", RecvBuf);
    printf("\r\nSocketClient end\r\n");
    closesocket(CliSock);
    WSACleanup();
    return 0;
}
