#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <winsock2.h>

SOCKET clientSocket;

static void stop(const char* msg)
{
	int err = WSAGetLastError();
	fprintf(stderr, "[%d] %s\n", err, msg);
	exit(EXIT_FAILURE);
}

static void initializeWinsock() 
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) 
		stop("WSAStartup failed");
}

static void initializeClientSocket()
{
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) 
		stop("Could not initialize socket for client");

	struct sockaddr_in socketAddress = {};
	socketAddress.sin_family = AF_INET;
	socketAddress.sin_port = htons(1234);
	socketAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	int connectionStatus = connect(clientSocket, (const struct sockaddr*)&socketAddress, sizeof(socketAddress));
	if (connectionStatus == SOCKET_ERROR) 
		stop("Connection to the server failed");
}

static void sendMessage(const char* msg)
{
	int messageLength = (int)strlen(msg);
	if (send(clientSocket, msg, messageLength, 0) == SOCKET_ERROR)
		stop("Sending message to server failed");
}

static void receiveMessage()
{
	char message[64] = {};
	int n = recv(clientSocket, message, sizeof(message) - 1, 0);

	if (n == SOCKET_ERROR) 
		stop("Receiving message from server failed");
	
	printf("Server says: %s\n", message);
}

int main() 
{
    initializeWinsock();
	initializeClientSocket();

	sendMessage("hello");
	receiveMessage();
    
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
