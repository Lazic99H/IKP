#define WIN32_LEAN_AND_MEAN
#pragma warning(disable:4996) // zbog inet_addr jer mi je pucao error
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define DEFAULT_BUFLEN 128
#define DEFAULT_PORT 27016

struct PublisherNotice {
    char topic[DEFAULT_BUFLEN];
    char message[DEFAULT_BUFLEN];
};

bool InitializeWindowsSockets();

int __cdecl main(int argc, char** argv)
{
    SOCKET connectSocket = INVALID_SOCKET;
  
    int iResult;

    if (InitializeWindowsSockets() == false)
    {
        return 1;
    }

    // create a socket
    connectSocket = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // create and initialize address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(DEFAULT_PORT);
    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }

    char topic[DEFAULT_BUFLEN];
    char message[DEFAULT_BUFLEN];

    PublisherNotice notice;

    do {
        printf("Enter topic: \n");
        gets_s(notice.topic, DEFAULT_BUFLEN);
        printf("Enter message you want to send to your subscribers \n");
        gets_s(notice.message, DEFAULT_BUFLEN);
        iResult = send(connectSocket, (char*)&notice, sizeof(notice), 0);

        if (iResult == SOCKET_ERROR)
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }

        printf("Bytes Sent: %ld\n", iResult);
        getchar();
    
    } while (true);
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}
