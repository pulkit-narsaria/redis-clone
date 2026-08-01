#include <winsock2.h>
#include <stdint.h>

class Client
{
public:
	Client();
	~Client();

	int32_t query();

private:
	static void stop(const char* message);
	static void dontStop(const char* message);
	static void initializeWinsock();
	void initializeClientSocket();
	static int32_t readMessage(SOCKET _clientSocket, char* message, int messageLength);
	static int32_t writeMessage(SOCKET fd, const char* buf, int n);
	static int32_t sendRequest(SOCKET _clientSocket, const char* message, size_t messageSize);
	static int32_t getResponse(SOCKET _clientSocket);

	SOCKET _clientSocket;
};