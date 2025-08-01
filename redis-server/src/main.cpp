#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <winsock2.h>

SOCKET serverSocket;

static void stop(const char* msg) 
{
	int err = WSAGetLastError();
	fprintf(stderr, "[%d] %s\n", err, msg);
	exit(EXIT_FAILURE);
}

static void dontStop(const char* msg)
{
	fprintf(stderr, "%s\n", msg);
}

static void initializeWinsock() 
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) 
        stop("WSAStartup failed");
}

static void initializeServerSocket()
{
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
		stop("Could not initialize socket for server");

	char optionVal = 1;
	int optionsStatus = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optionVal, sizeof(optionVal));
	if(optionsStatus == SOCKET_ERROR)
		stop("Could not properly set the options to the socket");

	struct sockaddr_in socketAddress = {};
	socketAddress.sin_family = AF_INET;
	socketAddress.sin_port = htons(1234);
	socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	int bindStatus = bind(serverSocket, (const struct sockaddr*)&socketAddress, sizeof(socketAddress));
	if (bindStatus == SOCKET_ERROR) 
		stop("Binding to the socket failed");

	int listenStatus = listen(serverSocket, SOMAXCONN);
	if (listenStatus == SOCKET_ERROR) 
		stop("Listening to the client failed");
}

static void processServerClientActions(SOCKET clientSocket) {
	char receivedMessage[64] = {};
	int receiveStatus = recv(clientSocket, receivedMessage, sizeof(receivedMessage) - 1, 0);

	if (receiveStatus == SOCKET_ERROR) 
	{
		dontStop("Receiving from the client failed");
		return;
	}

	fprintf(stderr, "Client says: %s\n", receivedMessage);

	char message[] = "world";
	int sendStatus = send(clientSocket, message, strlen(message), 0);
	if (sendStatus == SOCKET_ERROR)
		dontStop("Sending to the client failed");
}

static void connectToClient()
{
	while (true) 
	{
		struct sockaddr_in clientAddress = {};
		int addressLength = sizeof(clientAddress);
		SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &addressLength);
		if (clientSocket == INVALID_SOCKET) 
		{
			dontStop("Could not connect to the client");
			continue;
		}

		processServerClientActions(clientSocket);
		closesocket(clientSocket);
	}
}

int main() {
    initializeWinsock();
	initializeServerSocket();

	connectToClient();

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
