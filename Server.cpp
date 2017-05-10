#include <Ice/Ice.h>
#include <ChatI.h>

using namespace std;
using namespace Chat;

class Server : public Ice::Application
{
public:
	virtual int run(int, char*[]);
};

int
main(int argc, char* argv[])
{
	Server app;
	return app.main(argc, argv);
}

int
Server::run(int argc, char* argv[])
{
	shutdownOnInterrupt();
	
	Ice::ObjectAdapterPtr adapter = communicator()->
		createObjectAdapterWithEndpoints("Chat.Server", "default -h localhost -p 10000");
	
	ChatServerIPtr host = new ChatServerI;
	adapter->add(host, Ice::stringToIdentity("ChatHost"));
	adapter->activate();
	
	// Wait until we are done
	communicator()->waitForShutdown();
	if(interrupted())
	{
		cerr << appName() << ": interrupted, shutting down" << endl;
	}
	
	return EXIT_SUCCESS;
}
