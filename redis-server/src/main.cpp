#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <winsock2.h>

SOCKET serverSocket;
const size_t maxMessageSize = 4096;

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

	int bindStatus = bind(serverSocket, (const sockaddr*)&socketAddress, sizeof(socketAddress));
	if (bindStatus == SOCKET_ERROR) 
		stop("Binding to the socket failed");

	int listenStatus = listen(serverSocket, SOMAXCONN);
	if (listenStatus == SOCKET_ERROR) 
		stop("Listening to the client failed");
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

static int32_t getRequest(SOCKET connfd)
{
	char rbuf[4 + maxMessageSize];
	errno = 0;
	int32_t err = readMessage(connfd, rbuf, 4);
	if (err) {
		dontStop(errno == 0 ? "EOF" : "read() error");
		return err;
	}

	uint32_t len = 0;
	memcpy(&len, rbuf, 4);
	if (len > maxMessageSize) 
	{
		dontStop("too long");
		return -1;
	}

	err = readMessage(connfd, &rbuf[4], len);
	if (err) 
	{
		dontStop("read() error");
		return err;
	}

	fprintf(stderr, "client says: %.*s\n", len, &rbuf[4]);

	return 0;
}

static int32_t sendResponse(SOCKET connfd)
{
	const char reply[] = "world";
	char wbuf[4 + sizeof(reply)];
	uint32_t len = (uint32_t)strlen(reply);
	memcpy(wbuf, &len, 4);
	memcpy(&wbuf[4], reply, len);
	uint32_t err = writeMessage(connfd, wbuf, 4 + len);
	if (err)
	{
		dontStop("write() error");
		return err;
	}
	return 0;
}

static int32_t processRequest(SOCKET connfd) 
{
	// according to request response protocol server first gets the request from client then
	// responds to client with a response.

	int32_t err = getRequest(connfd);
	if (err)
	{
		dontStop("getRequest() error");
		return err;
	}

	return sendResponse(connfd);
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
	int sendStatus = send(clientSocket, message, (int)strlen(message), 0);
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

		while (true) 
		{
			int32_t err = processRequest(clientSocket);
			if (err) 
				break;
		}

		closesocket(clientSocket);
	}
}

int main() 
{
    initializeWinsock();
	initializeServerSocket();

	connectToClient();

    closesocket(serverSocket);
    WSACleanup();
}
