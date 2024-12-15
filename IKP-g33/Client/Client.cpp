#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 27016
#define BUFFER_SIZE 256

#define STRESS_TEST_THEADS_NUM 10
typedef struct ThreadArgs {
	SOCKET connectSocket;
} ThreadArgs;

void stress_test(SOCKET connectSocket) {
	char buffer[BUFFER_SIZE];
	char msg[] = "1,10\n";
	send(connectSocket, msg, (int)strlen(msg), 0);
	int iResult = recv(connectSocket, buffer, BUFFER_SIZE, 0);
	buffer[iResult] = '\0';
	printf(buffer);
}

// Thread function
unsigned __stdcall stress_test_thread(void* args) {
	ThreadArgs* threadArgs = (ThreadArgs*)args;
	SOCKET connectSocket = threadArgs->connectSocket;

	// Execute the stress_test function
	stress_test(connectSocket);

	return 0;
}

void run_stress_test(SOCKET connectSocket, int numThreads) {
	HANDLE threads[STRESS_TEST_THEADS_NUM];
	ThreadArgs threadArgs = { connectSocket };

	// Create multiple threads
	for (int i = 0; i < numThreads; i++) {
		threads[i] = CreateThread(
			NULL,                      // Security attributes
			0,                         // Stack size (default)
			(LPTHREAD_START_ROUTINE)stress_test_thread,        // Thread function
			&threadArgs,               // Arguments to thread function
			0,                         // Creation flags
			NULL                       // Thread identifier
		);

		if (threads[i] == NULL) {
			printf("Failed to create thread %d\n", i);
		}
		else {
			printf("Thread %d created successfully.\n", i);
		}
	}

	// Wait for all threads to finish
	WaitForMultipleObjects(numThreads, threads, TRUE, INFINITE);

	// Close thread handles
	for (int i = 0; i < numThreads; i++) {
		CloseHandle(threads[i]);
	}
}


bool isValidInteger(const char* str) {
	for (int i = 0; str[i] != '\0'; i++) {
		if (!isdigit(str[i])) {
			return false;
		}
	}
	return true;
}
// TCP client that use blocking sockets
int main()
{
	// Socket used to communicate with server
	SOCKET connectSocket = INVALID_SOCKET;

	// Variable used to store function return value
	int iResult;

	// Buffer we will use to store message
	char buffer[BUFFER_SIZE];

	// WSADATA data structure that is to receive details of the Windows Sockets implementation
	WSADATA wsaData;

	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
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

	// Create and initialize address structure
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;								// IPv4 protocol
	serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);	// ip address of server
	serverAddress.sin_port = htons(SERVER_PORT);					// server port

	// Connect to server specified in serverAddress and socket connectSocket
	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	//run_stress_test(connectSocket, STRESS_TEST_THEADS_NUM);

	while (strcmp(buffer, "x") != 0)
	{
		fseek(stdin, 0, SEEK_END);
		int command_code;
		printf("Choose a command:\n1. Allocate memory\n2. Free memory\nx. Quit\n");
		gets_s(buffer, BUFFER_SIZE);


		if (strcmp(buffer, "1") && strcmp(buffer, "2")) {
			printf("Invalid option. Please try again.\n");
			continue;
		}
		command_code = atoi(buffer);

		while (true) {
			if (command_code == 1) {
				printf("Enter size of memory you want to allocate:\n");
				gets_s(buffer, BUFFER_SIZE);
				if (!isValidInteger(buffer)) {
					continue;
				}
				break;
			}
			else if (command_code == 2) {
				printf("Enter the address that you want to free:\n");
				gets_s(buffer, BUFFER_SIZE);
				if (!isValidInteger(buffer)) {
					continue;
				}
				break;
			}
		}

		char command_code_str[2]; // 4 bytes: enough for single-digit numbers, a comma, and null terminator
		sprintf_s(command_code_str, sizeof(command_code_str), "%d", command_code);

		char formatted_message[BUFFER_SIZE];
		sprintf_s(formatted_message, sizeof(formatted_message), "%s,%s", command_code_str, buffer);


		iResult = send(connectSocket, formatted_message, (int)strlen(formatted_message), 0);
		fseek(stdin, 0, SEEK_END);
		fflush(stdin);

		iResult = recv(connectSocket, buffer, BUFFER_SIZE, 0);
		if (iResult > 0)
		{
			buffer[iResult] = '\0';
			printf(buffer);
		}
		else
		{
			printf("received failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}
	}

	// Shutdown the connection since we're done
	iResult = shutdown(connectSocket, SD_BOTH);

	// Check if connection is succesfully shut down.
	if (iResult == SOCKET_ERROR)
	{
		printf("Shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	closesocket(connectSocket);
	WSACleanup();

	return 0;
}