#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define MAX_CLIENT 2 
bool InitializeWindowsSockets();

int  main(void)
{
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;
    // Socket used for communication with client
    SOCKET acceptedSocket[MAX_CLIENT];

    int trenutniBrojKonekcija = 0;

    for (int i = 0; i < 2; i++) {
        acceptedSocket[i] = INVALID_SOCKET;
    }


    // variable used to store function return value
    int iResult;
    // Buffer used for storing incoming data
    char recvbuf[DEFAULT_BUFLEN];

    if (InitializeWindowsSockets() == false)
    {
        // we won't log anything since it will be logged
        // by InitializeWindowsSockets() function
        return 1;
    }

    // Prepare address information structures
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

    // Setup the TCP listening socket - bind port number and local address 
    // to socket
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
    while (true)
    {
        FD_ZERO(&readfds);
        printf("OVDJE SAM\n");
        Sleep(1000);
        if (trenutniBrojKonekcija < MAX_CLIENT) {
            FD_SET(listenSocket, &readfds);
            printf("PRVI IF\n");
            Sleep(500);

        }

        for (int i = 0; i < trenutniBrojKonekcija; i++)
        {
            FD_SET(acceptedSocket[i], &readfds);
            printf("PRVI FOR\n");
            Sleep(500);

        }

        //povratna vrijednost select funkcije je broj soketa koji su primili neki dogadjaj
        int result = select(0, &readfds, NULL, NULL, &timeVal);
        if (result == 0) {
            // vreme za cekanje je isteklo
            printf("ISTEKLO VRIJEME\n");
            Sleep(500);

        }
        else if (result == SOCKET_ERROR) {
            //desila se greska prilikom poziva funkcije
        }
        else {
            // rezultat je jednak broju soketa koji su zadovoljili uslov
            printf("Usli u else sad cemo da vidimo\n");
            Sleep(500);

            if (FD_ISSET(listenSocket, &readfds)) {
                printf("Usli u else pa if\n");
                Sleep(500);

                acceptedSocket[trenutniBrojKonekcija] = accept(listenSocket, NULL, NULL);

                if (acceptedSocket[trenutniBrojKonekcija] == INVALID_SOCKET)
                {
                    printf("accept failed with error: %d\n", WSAGetLastError());
                    closesocket(listenSocket);
                    WSACleanup();
                    return 1;
                }

                unsigned long mode = 1;
                iResult = ioctlsocket(acceptedSocket[trenutniBrojKonekcija], FIONBIO, &mode);//sad je u neblokirajucem rezimu
                trenutniBrojKonekcija++;
                printf("i namjestili ga u neblokirajuci\n");
                Sleep(500);


            }


            for (int i = 0; i < trenutniBrojKonekcija; i++) {
                printf("ZADNJI FOR\n");
                Sleep(500);
                if (acceptedSocket[i] != INVALID_SOCKET) {
                    printf("IF U ZADNJEM FORU\n");
                    Sleep(500);
                    iResult = recv(acceptedSocket[i], recvbuf, DEFAULT_BUFLEN, 0);
                    if (iResult > 0)
                    {
                        printf("Message received from client: %s.\n", recvbuf);
                    }
                    else if (iResult == 0)
                    {
                        // connection was closed gracefully
                        printf("Connection with client closed.\n");
                        closesocket(acceptedSocket[i]);

                        //trenitni broj konekcija -
                        //modifikacija niz aktivnih soketa
                    }
                    else
                    {
                        // there was an error during recv
                        printf("recv failed with error: %d\n", WSAGetLastError());
                        closesocket(acceptedSocket[i]);
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
