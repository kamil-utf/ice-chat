#include <Ice/Ice.h>
#include <ChatI.h>

using namespace std;
using namespace Chat;

class Manager : public Ice::Application
{
public:
	
	virtual int run(int, char*[]);
private:
	ChatServerPrx _host;
};

int
main(int argc, char* argv[])
{
	Manager app;
	return app.main(argc, argv, "config.manager");
}

int
Manager::run(int argc, char* argv[])
{
	shutdownOnInterrupt();
	
	Ice::ObjectPrx base = 
		communicator()->stringToProxy("ChatHost:default -h localhost -p 10000");
		
	_host = ChatServerPrx::checkedCast(base);
	if(!_host)
	{
		cerr << appName() << ": Invalid ChatServer proxy" << endl;
		return EXIT_FAILURE;
	}
	
	Ice::ObjectAdapterPtr adapter = communicator()->
		createObjectAdapterWithEndpoints("Chat.Manager", "default -h localhost");
		
	ChatGroupManagerIPtr manager = new ChatGroupManagerI;
	Ice::Identity ident = manager->ident();
	adapter->add(manager, ident);
	adapter->activate();
	
	ChatGroupManagerPrx managerPrx =
		ChatGroupManagerPrx::uncheckedCast(adapter->createProxy(ident));
	_host->addGroupManager(managerPrx);
	
	// Wait until we are done
	communicator()->waitForShutdown();
	if(interrupted())
	{
		cerr << appName() << ": interrupted, shutting down" << endl;
		_host->removeGroupManager(managerPrx);
	}
	
	return EXIT_SUCCESS;
}