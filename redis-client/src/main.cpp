#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <winsock2.h>

SOCKET clientSocket;
const size_t maxMessageSize = 4096;

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

	struct sockaddr_in serverAddress = {};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(1234);
	serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	int connectionStatus = connect(clientSocket, (const struct sockaddr*)&serverAddress, sizeof(serverAddress));
	if (connectionStatus == SOCKET_ERROR) 
		stop("Connection to the server failed");
}

static void dontStop(const char* msg)
{
	fprintf(stderr, "%s\n", msg);
}

static int32_t readMessage(SOCKET fd, char* buf, int n)
{
	while (n > 0) 
	{
		int readSize = recv(fd, buf, n,0);
		if (readSize <= 0) 
			return -1;
		assert(readSize <= n);
		n -= readSize;
		buf += readSize;
	}
	return 0;
}

static int32_t writeMessage(SOCKET fd, const char* buf, int n)
{
	while (n > 0) 
	{
		int writeSize = send(fd, buf, n,0);
		if (writeSize <= 0) 
			return -1;
		assert(writeSize <= n);
		n -= writeSize;
		buf += writeSize;
	}
	return 0;
}

static int32_t sendRequest(SOCKET clientSocket, const char* message)
{
	uint32_t len = (uint32_t)strlen(message);
	if (len > maxMessageSize)
		return -1;

	char sendMessage[4 + maxMessageSize];
	memcpy(sendMessage, &len, 4);
	memcpy(&sendMessage[4], message, len);

	int32_t err = writeMessage(clientSocket, sendMessage, 4 + len);
	if (err)
		return err;
	return 0;
}

static int32_t getResponse(SOCKET clientSocket)
{
	char getMessage[4 + maxMessageSize + 1];
	errno = 0;
	int32_t err = readMessage(clientSocket, getMessage, 4);

	if (err)
	{
		dontStop(errno == 0 ? "EOF" : "read() error");
		return err;
	}
		
	uint32_t len = 0;
	memcpy(&len, getMessage, 4);
	if (len > maxMessageSize)
	{
		dontStop("too long");
		return -1;
	}

	err = readMessage(clientSocket, &getMessage[4], len);
	if (err) {
		dontStop("read() error");
		return err;
	}

	printf("server says: %.*s\n", len, &getMessage[4]);
	return 0;
}

static int32_t query(SOCKET clientSocket, const char* message)
{
	// according to request response protocol first client sends request to the server 
	// then server processes the request and sends response back to the client

	int32_t err = sendRequest(clientSocket, message);
	if(err)
	{
		dontStop("sendRequest() error");
		return err;
	}

	err = getResponse(clientSocket);
	if(err)
	{
		dontStop("getResponse() error");
		return err;
	}
	
	return 0;
}

static void sendQueries()
{
	int32_t err;
	err = query(clientSocket, "hello1");
	if (err)
	{
		closesocket(clientSocket);
		return;
	}
	err = query(clientSocket, "hello2");
	if (err)
	{
		closesocket(clientSocket);
		return;
	}
	err = query(clientSocket, "hello3");
	if (err)
	{
		closesocket(clientSocket);
		return;
	}
}

int main() 
{
    initializeWinsock();
	initializeClientSocket();

	sendQueries();
	    
    closesocket(clientSocket);
    WSACleanup();

	return 0;
}
