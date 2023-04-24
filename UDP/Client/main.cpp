#include <iostream>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    printf("\r\nSocket Client begin\r\n");
    WSADATA wsaData{};
    SOCKET sock{};
    sockaddr_in SerAddr{};
    if (WSAStartup(MAKEWORD(2, 1), &wsaData) == -1) {
        printf("Error(WSAStartup) ErrorNumber(%d)\r\n", WSAGetLastError());
        return -1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1){
        printf("Error(WSAStartup) ErrorNumber(%d)\r\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    SerAddr.sin_family = PF_INET;
    SerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    SerAddr.sin_port = htons(6969);

    char SendBuf[]{"Hello Server!!!"};
    send(sock, SendBuf, sizeof(SendBuf), 0);

    char RecvBuf[1024]{};
    recv(sock, RecvBuf, sizeof(RecvBuf), 0);
    printf("Server Message: %s\r\n", RecvBuf);

    printf("Socket End");
    closesocket(sock);
    WSACleanup();
}