#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>

#define DEFAULT_BUFLEN 128
#define DEFAULT_PORT "27016"
#define BUFFER_SIZE 512
#define MAX_SERVER 2
#define MAX_CLIENT 10 

struct PublisherNotice {
    char topic[DEFAULT_BUFLEN];
    char message[DEFAULT_BUFLEN];
};


struct Queue {
    struct PublisherNotice data;
    struct Queue* next;
};


bool InitializeWindowsSockets();
void Decomposition(SOCKET* acceptedSockets, int indexDeleted, int indexCurrentSize);    
void initQueue(Queue** queue);
void Enqueue(Queue** queue, PublisherNotice data);
void Connect();
void Subscribe(void* topic);
void Publish(void* topic, void* message);
PublisherNotice* Dequeue(Queue** queue);


int main(void)
{

    /*SOCKET listenSocket[MAX_SERVER];*/
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET acceptedSocket[MAX_CLIENT];
    char recvbuf[BUFFER_SIZE];
    int iResult;
    int currentConncetionCounter = 0;
    Queue* queue;
    initQueue(&queue);
  /*for (int i = 0; i < MAX_SERVER; i++) {
        listenSocket[i] = INVALID_SOCKET;
    }*/

    for (int i = 0; i < 2; i++) {
        acceptedSocket[i] = INVALID_SOCKET;
    }

    if (InitializeWindowsSockets() == false){     
        return 1;
    }

    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    listenSocket = socket(AF_INET,      // IPv4 address famly
        SOCK_STREAM,  // stream socket
        IPPROTO_TCP); // TCP

    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return 1;
    }


    iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    // Set listenSocket in listening mode
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    unsigned long mode = 1;
    iResult = ioctlsocket(listenSocket, FIONBIO, &mode);
    printf("Server initialized, waiting for clients.\n");

    fd_set readfds;

    timeval timeVal;
    timeVal.tv_sec = 1;
    timeVal.tv_usec = 0;
   //
   // ovo je koristeno da u prvom elsu accept umjesto drugog i trece argumenta se prosljede clinet i ovaj &size nz sto, ustvari vjr da se sacuvaju tu podaci tog klijenta odakle je itd msm koji mu je port ovo ono mozda
   //sockaddr_in client;
   // int size = sizeof(client);

    FD_ZERO(&readfds);
    
    while (true)
    {

        if (currentConncetionCounter < MAX_CLIENT) {
            FD_SET(listenSocket, &readfds);
        }

        for (int i = 0; i < currentConncetionCounter; i++){
            FD_SET(acceptedSocket[i], &readfds);
        }

        //povratna vrijednost select funkcije je broj soketa koji su primili neki dogadjaj
        int result = select(0, &readfds, NULL, NULL, &timeVal);
        if (result == 0) {
            printf("Time expired\n");

            continue;
        }
        else if (result == SOCKET_ERROR) {
            printf("Listen socket error\n");
            closesocket(listenSocket);
            WSACleanup();
            return -1;
        }
        else {
            // rezultat je jednak broju soketa koji su zadovoljili uslov
            if (FD_ISSET(listenSocket, &readfds)) {

                acceptedSocket[currentConncetionCounter] = accept(listenSocket, NULL, NULL);

                if (acceptedSocket[currentConncetionCounter] == INVALID_SOCKET)
                {
                    printf("accept failed with error: %d\n", WSAGetLastError());
                    closesocket(listenSocket);
                    WSACleanup();
                    return 1;
                }

                unsigned long mode = 1;
                iResult = ioctlsocket(acceptedSocket[currentConncetionCounter], FIONBIO, &mode);//sad je u neblokirajucem rezimu
                currentConncetionCounter++;

            }

            for (int i = 0; i < currentConncetionCounter; i++) {
                if (FD_ISSET(acceptedSocket[i], &readfds)) {
                    iResult = recv(acceptedSocket[i], recvbuf, BUFFER_SIZE, 0);
                    if (iResult > 0) {
                        PublisherNotice* recvNotice = (PublisherNotice*)recvbuf;
                        printf("Topic: %s\n Message: %s\n", recvNotice->topic,recvNotice->message);

                        //ovdje napraviti thread koji dodaje u Queue
                    }
                    else if (iResult == 0){
                        // connection was closed gracefully
                        printf("Connection with client closed.\n");
                        closesocket(acceptedSocket[i]);
                        Decomposition(acceptedSocket, i, currentConncetionCounter);
                    }
                    else if (iResult == SOCKET_ERROR) {
                        // there was an error during recv
                        printf("recv failed with error: %d\n", WSAGetLastError());
                        closesocket(acceptedSocket[i]);
                        Decomposition(acceptedSocket, i, currentConncetionCounter);
                    }
                }
            }

        }
    }

    //cleanup accSockets
    for (int i = 0; i < MAX_CLIENT; i++) {
        // shutdown the connection since we're done
        iResult = shutdown(acceptedSocket[i], SD_SEND);
        if (iResult == SOCKET_ERROR)
        {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(acceptedSocket[i]);
            WSACleanup();
            return 1;
        }

        closesocket(acceptedSocket[i]);
    }

    // cleanup
    closesocket(listenSocket);
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
void initQueue(Queue** queue) {
    *queue = NULL;
}
void Enqueue(Queue** queue, PublisherNotice data) {

    Queue* newQueue = (Queue*)malloc(sizeof(Queue));
    newQueue->data = data;
    newQueue->next = NULL;

    if (*queue == NULL)
        *queue = newQueue;

    Queue* pom = *queue;
    while (pom->next != NULL) {
        pom = pom->next;
    }

    pom->next = newQueue;
}
PublisherNotice* Dequeue(Queue** queue) {

    PublisherNotice* ret = (PublisherNotice*)malloc(sizeof(PublisherNotice));

    Queue* pom = *queue;

    if (pom == NULL)
        return NULL;

    strcpy_s(ret->topic, pom->data.topic);
    strcpy_s(ret->message, pom->data.message);

    *queue = pom->next;

    free(pom);

    return ret;
}
void Decomposition(SOCKET* acceptedSockets, int indexDeleted, int indexCurrentSize)
{
    for (int i = indexDeleted; i < indexCurrentSize - 1; i++) {
        acceptedSockets[i] = acceptedSockets[i + 1];
    }
}


//TASKS(Not finished)
void Connect();
void Subscribe(void* topic);
void Publish(void* topic, void* message);