#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <winsock2.h>
#include <vector>

SOCKET serverSocket;
const size_t maxMessageSize = 32 << 20;

struct ClientConnection 
{
	SOCKET socket = -1;

	// application's intention, for the event loop
	bool want_read = false;
	bool want_write = false;
	bool want_close = false;

	std::vector<char> clientMessage;
	std::vector<char> serverResponseMessage;
};

static void stop(const char* message) 
{
	int err = WSAGetLastError();
	fprintf(stderr, "[%d] %s\n", err, message);
	exit(EXIT_FAILURE);
}

static void dontStop(const char* message)
{
	fprintf(stderr, "%s\n", message);
}

static void addToMessage(std::vector<char>& messageBuffer, const char* message, size_t messageSize)
{
	messageBuffer.insert(messageBuffer.end(), message, message + messageSize);
}

static void removeFromMessage(std::vector<char>& messageBuffer, size_t messageSize)
{
	messageBuffer.erase(messageBuffer.begin(), messageBuffer.begin() + messageSize);
}

static void initializeWinsock() 
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) 
        stop("WSAStartup failed");
}

static void setSocketToNonBlockingIo(SOCKET &socket)
{
	u_long mode = 1;
	if (ioctlsocket(socket, FIONBIO, &mode) != 0)
		stop("ioctlsocket error");
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

	setSocketToNonBlockingIo(serverSocket);

	int listenStatus = listen(serverSocket, SOMAXCONN);
	if (listenStatus == SOCKET_ERROR) 
		stop("Listening to the client failed");
}

static void writeMessage(ClientConnection* clientConnection)
{
	assert(clientConnection->serverResponseMessage.size() > 0);
	size_t messageLength = send(clientConnection->socket, &clientConnection->serverResponseMessage[0], (int)clientConnection->serverResponseMessage.size(), 0);
	
	if (messageLength < 0 && errno == EAGAIN) return;
	
	if (messageLength < 0) 
	{
		dontStop("write() error");
		clientConnection->want_close = true;
		return;
	}

	removeFromMessage(clientConnection->serverResponseMessage, (size_t)messageLength);

	if (clientConnection->serverResponseMessage.size() == 0) 
	{
		clientConnection->want_read = true;
		clientConnection->want_write = false;
	}
}

static bool processOneReadRequest(ClientConnection* clientConnection) 
{
	if (clientConnection->clientMessage.size() < 4) return false;
	
	uint32_t clientMessageLength = 0;
	memcpy(&clientMessageLength, clientConnection->clientMessage.data(), 4);

	if (clientMessageLength > maxMessageSize) 
	{
		dontStop("too long");
		clientConnection->want_close = true;
		return false;
	}

	if (4 + clientMessageLength > clientConnection->clientMessage.size()) return false;
	
	const char* clientMessageText = &clientConnection->clientMessage[4];

	printf("client says: len:%d data:%.*s\n", clientMessageLength, clientMessageLength < 100 ? clientMessageLength : 100, clientMessageText);

	addToMessage(clientConnection->serverResponseMessage, (const char*)&clientMessageLength, 4);
	addToMessage(clientConnection->serverResponseMessage, clientMessageText, clientMessageLength);

	removeFromMessage(clientConnection->clientMessage, 4 + clientMessageLength);
	
	return true;
}

static void readMessage(ClientConnection* clientConnection)
{
	char clientMessage[64 * 1024];
	size_t rv = recv(clientConnection->socket, clientMessage, sizeof(clientMessage), 0);

	if (rv < 0 && errno == EAGAIN) return;
	
	if (rv < 0) 
	{
		dontStop("read() error");
		clientConnection->want_close = true;
		return;
	}

	if (rv == 0) 
	{
		clientConnection->clientMessage.size() == 0 ? dontStop("client closed") : stop("unexpected EOF");
		clientConnection->want_close = true;
		return;
	}

	addToMessage(clientConnection->clientMessage, clientMessage, (size_t)rv);

	while (processOneReadRequest(clientConnection)) {}
	
	if (clientConnection->serverResponseMessage.size() > 0) 
	{
		clientConnection->want_read = false;
		clientConnection->want_write = true;
		return writeMessage(clientConnection);
	}
}

static ClientConnection* acceptClientConnection(SOCKET socket) 
{
	struct sockaddr_in clientSocketAddress = {};
	int addrlen = sizeof(clientSocketAddress);
	SOCKET connfd = accept(socket, (struct sockaddr*)&clientSocketAddress, &addrlen);
	
	if (connfd < 0) 
	{
		dontStop("accept() error");
		return NULL;
	}

	uint32_t ip = clientSocketAddress.sin_addr.s_addr;
	
	fprintf(stderr, "new client from %u.%u.%u.%u:%u\n", ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24, ntohs(clientSocketAddress.sin_port));

	setSocketToNonBlockingIo(connfd);

	ClientConnection* clientConnection = new ClientConnection();
	clientConnection->socket = connfd;
	clientConnection->want_read = true;
	return clientConnection;
}

static void handleClientConnections(std::vector<struct pollfd>& poll_args, std::vector<ClientConnection*>& clientConnections)
{
	if (poll_args[0].revents)
	{
		if (ClientConnection* clientConnection = acceptClientConnection(serverSocket))
		{
			if (clientConnections.size() <= (size_t)clientConnection->socket)
				clientConnections.resize(clientConnection->socket + 1);

			assert(!clientConnections[clientConnection->socket]);
			clientConnections[clientConnection->socket] = clientConnection;
		}
	}
}

static bool setPollArgs(std::vector<struct pollfd> &poll_args,std::vector<ClientConnection*> &clientConnections)
{
	poll_args.clear();

	struct pollfd serverPollFd = { serverSocket, POLLIN, 0 };
	poll_args.push_back(serverPollFd);

	for (const auto& client : clientConnections)
	{
		if (!client) continue;

		struct pollfd clientPollFd = { client->socket, 0, 0 };

		if (client->want_read) clientPollFd.events |= POLLIN;
		if (client->want_write) clientPollFd.events |= POLLOUT;

		poll_args.push_back(clientPollFd);
	}

	// wait for readiness
	int rv = WSAPoll(poll_args.data(), (u_long)poll_args.size(), -1);

	if (rv < 0 && errno == EINTR) return false;
	if (rv < 0) stop("poll");

	return true;
}

static void handleClientEvents(std::vector<struct pollfd>& poll_args, std::vector<ClientConnection*>& clientConnections)
{
	for (size_t i = 1; i < poll_args.size(); ++i)
	{
		uint32_t ready = poll_args[i].revents;

		if (ready == 0)
		{
			continue;
		}

		ClientConnection* clientConnection = clientConnections[poll_args[i].fd];

		if (ready & POLLIN)
		{
			assert(clientConnection->want_read);
			readMessage(clientConnection);
		}

		if (ready & POLLOUT)
		{
			assert(clientConnection->want_write);
			writeMessage(clientConnection);
		}

		if ((ready & POLLERR) || clientConnection->want_close) {
			(void)closesocket(clientConnection->socket);
			clientConnections[clientConnection->socket] = NULL;
			delete clientConnection;
		}
	}
}

static void handleClients()
{
	std::vector<ClientConnection*> clientConnections;
	std::vector<struct pollfd> poll_args;

	while (true) 
	{
		setPollArgs(poll_args, clientConnections);
		handleClientConnections(poll_args, clientConnections);
		handleClientEvents(poll_args, clientConnections);
	}
}

int main() 
{
    initializeWinsock();
	initializeServerSocket();

	handleClients();

    closesocket(serverSocket);
    WSACleanup();

	return 0;
}
