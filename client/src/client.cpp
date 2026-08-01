#include "client.h"
#include <assert.h>
#include <errno.h>
#include <vector>
#include <string>


const size_t MaxMessageSize = 32 << 20;

Client::Client()
{
	initializeWinsock();
	initializeClientSocket();
}

Client::~Client()
{
	closesocket(_clientSocket);
	WSACleanup();
}

void Client::stop(const char* msg)
{
	int err = WSAGetLastError();
	fprintf(stderr, "[%d] %s\n", err, msg);
	exit(EXIT_FAILURE);
}

void Client::dontStop(const char* message)
{
	fprintf(stderr, "%s\n", message);
}

void Client::initializeWinsock()
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
		stop("WSAStartup failed");
}

void Client::initializeClientSocket()
{
	_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_clientSocket == INVALID_SOCKET)
		stop("Could not initialize socket for client");

	struct sockaddr_in serverAddress = {};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(1234);
	serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	int connectionStatus = connect(_clientSocket, (const struct sockaddr*)&serverAddress, sizeof(serverAddress));
	if (connectionStatus == SOCKET_ERROR)
		stop("Connection to the server failed");
}

int32_t Client::readMessage(SOCKET _clientSocket, char* message, int messageLength)
{
	while (messageLength > 0)
	{
		size_t readSize = recv(_clientSocket, message, messageLength, 0);
		if (readSize <= 0) return -1;
		assert((size_t)readSize <= messageLength);
		messageLength -= readSize;
		message += readSize;
	}
	return 0;
}

int32_t Client::writeMessage(SOCKET fd, const char* buf, int n)
{
	while (n > 0)
	{
		int writeSize = send(fd, buf, n, 0);
		if (writeSize <= 0)
			return -1;
		assert(writeSize <= n);
		n -= writeSize;
		buf += writeSize;
	}
	return 0;
}

int32_t Client::sendRequest(SOCKET _clientSocket, const char* message, size_t messageSize)
{
	if (messageSize > MaxMessageSize) {
		return -1;
	}

	std::vector<char> sendMessage;
	sendMessage.insert(sendMessage.end(), (char*)&messageSize, (char*)&messageSize + 4);
	sendMessage.insert(sendMessage.end(), message, message + messageSize);

	return writeMessage(_clientSocket, sendMessage.data(), (int)sendMessage.size());
}

int32_t Client::getResponse(SOCKET _clientSocket)
{
	std::vector<char> getMessage;
	getMessage.resize(4);
	errno = 0;
	int32_t err = readMessage(_clientSocket, &getMessage[0], 4);
	if (err)
	{
		dontStop(errno == 0 ? "EOF" : "read() error");
		return err;
	}

	uint32_t len = 0;
	memcpy(&len, getMessage.data(), 4);
	if (len > MaxMessageSize)
	{
		dontStop("too long");
		return -1;
	}

	getMessage.resize(4 + len);
	err = readMessage(_clientSocket, &getMessage[4], len);
	if (err) {
		dontStop("read() error");
		return err;
	}

	// do something
	printf("len:%u data:%.*s\n", len, len < 100 ? len : 100, &getMessage[4]);
	return 0;
}

int32_t Client::query()
{
	// according to request response protocol first client sends request to the server 
	// then server processes the request and sends response back to the client

	std::vector<std::string> query_list = { "hello1", "hello2", "hello3", std::string(MaxMessageSize, 'z'), "hello5", };

	for (const std::string& s : query_list)
	{
		int32_t err = sendRequest(_clientSocket, (char*)s.data(), s.size());
		if (err)
		{
			dontStop("sendRequest() error");
			return err;
		}
	}

	for (size_t i = 0; i < query_list.size(); ++i)
	{
		int32_t err = getResponse(_clientSocket);
		if (err)
		{
			dontStop("getResponse() error");
			return err;
		}
	}

	return 0;
}
