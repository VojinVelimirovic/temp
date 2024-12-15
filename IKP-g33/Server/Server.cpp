#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "../HeapManager/MemorySegment.h"
#include "../HeapManager/Memory.h"
#include "../HeapManager/Queue.h"

#define SERVER_PORT 27016
#define BUFFER_SIZE 256
#define MAX_CLIENTS 5

// Atomic flag for server shutdown
volatile LONG serverRunning = 1;


// Function for worker threads to process requests
DWORD WINAPI processRequest(LPVOID param) {
	Queue* queue = (Queue*)param;
	DWORD threadId = GetCurrentThreadId(); // Get the current thread ID

	while (serverRunning /*|| queue->front != NULL*/) {
		Request request;
		if (dequeue(queue, &request)) {
			char responseBuffer[BUFFER_SIZE]; // Buffer to send responses back to the client

			// Log which thread is processing the request
			if (request.isAllocate) {
				printf("Thread ID %lu: Processing allocation request for %d bytes.\n", threadId, request.value);
				int allocatedBlockId = (int)allocate_memory(request.value);
				if (allocatedBlockId != -1) {
					printf("Thread ID %lu: SUCCESS: Allocated %d bytes at id: %d\n\n", threadId, request.value, allocatedBlockId);
					snprintf(responseBuffer, BUFFER_SIZE, "SUCCESS: Allocated %d bytes at id: %d\n\n", request.value, allocatedBlockId);
				}
				else {
					printf("ERROR: Memory allocation failed for %d bytes.\n", request.value);
					strcpy_s(responseBuffer, "ERROR: Memory allocation failed.");
				}
			}
			else {
				printf("Thread ID %lu: Processing deallocation request for id: %d.\n", threadId, request.value);
				free_memory((void*)request.value);
				if (free_memory_error == 0) {
					printf("Thread ID %lu: SUCCESS: Memory freed at id: %d\n", threadId, request.value);
					snprintf(responseBuffer, BUFFER_SIZE, "SUCCESS: Memory freed at id: %d\n\n", request.value);
				}
				else {
					printf("ERROR: Failed to free memory at id: %d.\n", request.value);
					snprintf(responseBuffer, BUFFER_SIZE, "ERROR: Failed to free memory at id: %d.\n\n", request.value);
				}
			}

			drawMemorySegments();
            //drawMemorySegments2();

			// Send the response back to the client
			if (send(request.clientSocket, responseBuffer, (int)strlen(responseBuffer), 0) == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
			}
		}
	}
	return 0;
}


int main()
{
    initializeMemory(5);

    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSockets[MAX_CLIENTS] = { 0 };  // Track connected clients
    int iResult;
    char dataBuffer[BUFFER_SIZE];

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }

    sockaddr_in serverAddress;
    memset((char*)&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set the listening socket to non-blocking mode
    u_long nonBlockingMode = 1;
    ioctlsocket(listenSocket, FIONBIO, &nonBlockingMode);

    iResult = bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server socket is set to non-blocking listening mode. Waiting for new connection requests.\n");

    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(struct sockaddr_in);

    // Initialize the queue
    Queue requestQueue;
    initializeQueue(&requestQueue);

    // Create worker threads
    const int threadPoolSize = 4;
    HANDLE* threadPool = (HANDLE*)malloc(threadPoolSize * sizeof(HANDLE));
    for (int i = 0; i < threadPoolSize; ++i) {
        threadPool[i] = CreateThread(NULL, 0, processRequest, (LPVOID)&requestQueue, 0, NULL);
    }


    fd_set readfds;
    char inputBuffer[BUFFER_SIZE];
    while (serverRunning) {
        FD_ZERO(&readfds);
        FD_SET(listenSocket, &readfds);

        SOCKET maxSocket = listenSocket;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clientSockets[i] > 0) {
                FD_SET(clientSockets[i], &readfds);
                if (clientSockets[i] > maxSocket) {
                    maxSocket = clientSockets[i];
                }
            }
        }

        timeval timeout = { 1, 0 };  // 1-second timeout for select
        int activity = select((int)maxSocket + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0 && WSAGetLastError() != WSAEINTR) {
            printf("select error: %d\n", WSAGetLastError());
            break;
        }

        // Handle new connections
        if (FD_ISSET(listenSocket, &readfds)) {
            SOCKET newSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
            if (newSocket != INVALID_SOCKET) {
                printf("\nNew client connected: %s : %d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                ioctlsocket(newSocket, FIONBIO, &nonBlockingMode);  // Set the new socket to non-blocking mode

                for (int i = 0; i < MAX_CLIENTS; ++i) {
                    if (clientSockets[i] == 0) {
                        clientSockets[i] = newSocket;
                        break;
                    }
                }
            }
        }

        // Handle IO on connected clients
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            SOCKET clientSocket = clientSockets[i];
            if (clientSocket > 0 && FD_ISSET(clientSocket, &readfds)) {
                iResult = recv(clientSocket, dataBuffer, BUFFER_SIZE, 0);
                if (iResult > 0) {
                    dataBuffer[iResult] = '\0';

                    Request newRequest;
                    newRequest.clientSocket = clientSocket;

                    char* token = strtok(dataBuffer, ",");
                    newRequest.isAllocate = (atoi(token) == 1);

                    token = strtok(NULL, ",");
                    newRequest.value = atoi(token);

                    enqueue(&requestQueue, newRequest);
                    WakeConditionVariable(&requestQueue.notEmpty);
                }
                else if (iResult == 0 || WSAGetLastError() != WSAEWOULDBLOCK) {
                    printf("Client disconnected or error: %d\n", WSAGetLastError());
                    closesocket(clientSocket);
                    clientSockets[i] = 0;
                }
            }
        }

        // Check for user input
        if (_kbhit()) {  // Check if a key was pressed
            fgets(inputBuffer, sizeof(inputBuffer), stdin); // Read the input
            if (strcmp(inputBuffer, "draw\n") == 0) {
                drawMemorySegments(); // Call your function here
                //drawMemorySegments2();
            }
        }
    }

    // Cleanup
    serverRunning = 0;
    for (int i = 0; i < threadPoolSize; i++) {
        WakeConditionVariable(&requestQueue.notEmpty);
        WaitForSingleObject(threadPool[i], INFINITE);
        CloseHandle(threadPool[i]);
    }
    free(threadPool);
    freeQueue(&requestQueue);
    closesocket(listenSocket);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clientSockets[i] > 0) {
            closesocket(clientSockets[i]);
        }
    }
    WSACleanup();
    cleanup_segments();

    return 0;
}