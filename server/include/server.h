#include <winsock2.h>
#include <stdint.h>
#include <vector>

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

class Server
{
public:
	Server();
	~Server();

	void handleClients();
private:
	static void stop(const char* message);
	static void dontStop(const char* message);
	static void initializeWinsock();
	static void addToMessage(std::vector<char>& messageBuffer, const char* message, size_t messageSize);
	static void removeFromMessage(std::vector<char>& messageBuffer, size_t messageSize);
	static void setSocketToNonBlockingIo(SOCKET& socket);
	void initializeServerSocket();
	static void writeMessage(ClientConnection* clientConnection);
	static bool processOneReadRequest(ClientConnection* clientConnection);
	static void readMessage(ClientConnection* clientConnection);
	static ClientConnection* acceptClientConnection(SOCKET socket);
	void handleClientConnections(std::vector<struct pollfd>& poll_args, std::vector<ClientConnection*>& clientConnections);
	bool setPollArgs(std::vector<struct pollfd>& poll_args, std::vector<ClientConnection*>& clientConnections);
	static void handleClientEvents(std::vector<struct pollfd>& poll_args, std::vector<ClientConnection*>& clientConnections);

	SOCKET _serverSocket;
};
