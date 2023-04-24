#include <iostream>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    printf("\r\nSocket Client begin\r\n");
    WSADATA wsaData{};
    SOCKET sock{};
    sockaddr_in SerAddr{}, CliAddr{};
    if (WSAStartup(MAKEWORD(2, 1), &wsaData) == -1) {
        printf("Error(WSAStartup) ErrorNumber(%d)\r\n", WSAGetLastError());
        return -1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1){
        printf("Error(socket) ErrorNumber(%d)\r\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    SerAddr.sin_family = PF_INET;
    SerAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    SerAddr.sin_port = htons(6969);

    if(bind(sock, (SOCKADDR*)&SerAddr, sizeof(SOCKADDR)) == SOCKET_ERROR){
        printf("Error(bind) ErrorNumber(%d)\r\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    char RecvBuf[1024]{};
    int len = sizeof(CliAddr);
    recvfrom(sock, RecvBuf, sizeof(RecvBuf), 0, (SOCKADDR*)&CliAddr,&len);

    printf("Client Message: %s\r\n", RecvBuf);

    char SendBuf[]{"Hello Client!!!"};
    sendto(sock, SendBuf, sizeof(SendBuf), 0, (SOCKADDR*)&CliAddr, len);


    printf("Socket End");
    closesocket(sock);
    WSACleanup();
}
