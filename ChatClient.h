#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include <Ice/Ice.h>
#include <Chat.h>
#include <boost/program_options.hpp>
#include <boost/assign.hpp>
#include <map>

#define MIN_USERNAME_LEN		3
#define MIN_GROUP_NAME_LEN		4

namespace boost_po = boost::program_options;
namespace boost_as = boost::assign;

////////// ChatCallbackI ////////////////////

class ChatCallbackI : virtual public Chat::ChatCallback
{
public:
    virtual void receive(const std::string&,
                         const Ice::Current&);
};

typedef IceUtil::Handle<ChatCallbackI> ChatCallbackIPtr;

////////// ChatClient ////////////////////

class ChatClient : public Ice::Application
{
public:
	void userList();
	
	void createGroup(const std::string&);
	void deleteGroup(const std::string&);
	void groupList();
	
	void joinGroup(const std::string&);
	void leaveGroup(const std::string&);
	
	void sendMessage(const std::string&,
					 const std::string&);
	void sendPMessage(const std::string&,
					  const std::string&);
	
	std::string appIntro();
	std::string appPrompt();
	
	virtual int run(int, char*[]);
private:
	void commandLine();
	std::string signUp();
	
	bool isMyOwnGroup(const std::string&);
	
	static void parseOptions(boost_po::variables_map&,
							 const std::vector<std::string>&,
							 const std::string&);
	static std::map<std::string, boost_po::options_description> cmd_opts;
	
	enum Role { owner, member };
	typedef std::map<std::string, Role> MapGroup;
	
	Chat::ChatServerPrx _host;
	Chat::ChatUserPrx _self;
	MapGroup _myGroups;
};

#endif