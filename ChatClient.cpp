#include <ChatClient.h>
#include <ChatUtils.h>
#include <IceUtil/UUID.h>

using namespace std;
using namespace Chat;

////////// ChatCallbackI ////////////////////

void
ChatCallbackI::receive(const string& message,
					   const Ice::Current& c)
{
	cout << endl << message << endl;
}

////////// ChatClient ////////////////////

namespace {
	std::map<std::string, boost_po::options_description>
	initOptionsGroups()
	{
		/* Base options */
		boost_po::options_description base("Usage");
		base.add_options()
			("action,a", boost_po::value<string>(), "Action for IceChat")
			("help,h", "Display this help message")
			("quit,q", "Quit");
			
		/* User options */
		boost_po::options_description user;
		user.add_options()
			("user,u", boost_po::value<string>()->required());
			
		/* Group options */
		boost_po::options_description group;
		group.add_options()
			("group,g", boost_po::value<string>()->required());
			
		/* Message options */
		boost_po::options_description message;
		message.add_options()
			("message,m", boost_po::value<string>()->required());
			
		/* Group message options */
		boost_po::options_description gmsg;
		gmsg.add(message).add(group);
		
		/* Private message options */
		boost_po::options_description pmsg;
		pmsg.add(message).add(user);
			
		return boost_as::map_list_of
			("base_opts", base) ("user_opts", user)
			("group_opts", group) ("msg_opts", message)
			("gmsg_opts", gmsg) ("pmsg_opts", pmsg);
	}
}

std::map<std::string, boost_po::options_description> ChatClient::cmd_opts = initOptionsGroups();

void
ChatClient::userList()
{
	Users users = _host->getUserList();
	for(Users::const_iterator i = users.begin(); i != users.end(); ++i)
	{
		ChatUserPrx usr = ChatUserPrx::checkedCast(*i);
		cout << usr->name() << endl;
	}
}

void
ChatClient::createGroup(const string& name)
{
	try
	{
		_host->createGroup(name);
		_host->findGroupByName(name)->join(_self);
		
		_myGroups[name] = owner;
	}
	catch(const NameAlreadyExists&)
	{
		cout << "Group " << name << " already exists." << endl;
	}
	catch(const NameDoesNotExist&)
	{
		cout << "Group " << name << " does not exist." << endl;
	}
}

void
ChatClient::deleteGroup(const string& name)
{
	if(!isMyOwnGroup(name))
	{
		cout << "Permission denied. You are not the owner of " << name << " group." << endl;
		return;
	}
	
	try
	{
		_host->deleteGroup(name);
		
		MapGroup::iterator pos = _myGroups.find(name);
		if(pos != _myGroups.end())
		{
			_myGroups.erase(pos);
		}
	}
	catch(const NameDoesNotExist&)
	{
		cout << "Group " << name << " does not exist." << endl;
	}
}

void
ChatClient::groupList()
{
	Groups groups = _host->getGroupList();
	for(Groups::const_iterator i = groups.begin(); i != groups.end(); ++i)
	{
		ChatGroupPrx gr = ChatGroupPrx::checkedCast(*i);
		string groupName = gr->name();
		cout << groupName << (isMyOwnGroup(groupName) ? " (owner)" : "") << endl; 
	}
}

void
ChatClient::joinGroup(const string& name)
{
	try
	{
		_host->findGroupByName(name)->join(_self);
		_myGroups[name] = member;
	}
	catch(const NameDoesNotExist&)
	{
		cout << "Group " << name << " does not exist." << endl;
	}
	catch(const UserAlreadyExists&)
	{
		cout << "You are already a member of " << name << " group." << endl;
	}
}

void
ChatClient::leaveGroup(const string& name)
{
	try
	{
		_host->findGroupByName(name)->leave(_self);
	}
	catch(const NameDoesNotExist&)
	{
		cout << "Group " << name << " does not exist." << endl;
	}
	catch(const UserDoesNotExist&)
	{
		cout << "You are not a member of " << name << " group." << endl;
	}
}

void
ChatClient::sendMessage(const string& msg, const string& group)
{
	try
	{
		_host->findGroupByName(group)->sendMessage(msg, _self);
	}
	catch(const NameDoesNotExist&)
	{
		cout << "Group " << group << " does not exist." << endl;
	}
}

void
ChatClient::sendPMessage(const string& msg, const string& username)
{
	try
	{
		_host->findUserByName(username)->sendPMessage(msg, _self);
	}
	catch(const NameDoesNotExist&)
	{
		cout << "User " << username << " does not exist.";
	}
}

string
ChatClient::appIntro()
{
	return "IceChat\nCommand Line Interface (CLI)\n\n";
}

string
ChatClient::appPrompt()
{
	return "icechat > ";
}

int
ChatClient::run(int argc, char* argv[])
{
	Ice::ObjectPrx base = 
		communicator()->stringToProxy("ChatHost:default -h localhost -p 10000")
		->ice_twoway()->ice_timeout(-1)->ice_secure(false);
		
	_host = ChatServerPrx::checkedCast(base);
	if(!_host)
	{
		cerr << appName() << ": Invalid ChatServer proxy" << endl;
		return EXIT_FAILURE;
	}
	
	signUp();
	
	Ice::ObjectAdapterPtr adapter = communicator()->
		createObjectAdapterWithEndpoints("Chat.Client", "default -h localhost");
	
	Ice::Identity ident;
	ident.name = IceUtil::generateUUID();
	ChatCallbackIPtr callback = new ChatCallbackI;
	adapter->add(callback, ident);
	adapter->activate();
	
	ChatCallbackPrx callbackPrx = 
		ChatCallbackPrx::uncheckedCast(adapter->createProxy(ident));
	_self->setCallback(callbackPrx);
		
	commandLine();
	
	for(MapGroup::const_iterator i = _myGroups.begin(); i != _myGroups.end(); ++i)
	{
		string name = i->first;
		try
		{
			if(i->second == owner)
				_host->deleteGroup(name);
			else if(i->second == member)
				_host->findGroupByName(name)->leave(_self);
		}
		catch(const NameDoesNotExist&)
		{
			continue;
		}
	}
	_self->destroy();
	
	return EXIT_SUCCESS;
}

string
ChatClient::signUp()
{
	string username;
	bool is_valid = false;
	
	cout << appIntro();
	do
	{
		cout << "Your username: " << flush;
		getline(cin, username);
		username = ChatUtils::trim(username);
		
		if(username.length() < MIN_USERNAME_LEN)
		{
			cout << "Username must be at least " << MIN_USERNAME_LEN << " characters" << endl;
			continue;
		}
		
		try
		{
			_self = _host->login(username);
			is_valid = true;
		}
		catch(const NameAlreadyExists&)
		{
			cout << "User " << username << " already exists." << endl;
		}
	}
	while(!is_valid);
	
	return username;
}

void
ChatClient::commandLine()
{
	string cmdLine;
	boost_po::variables_map vmap;
	vector<string> args, unrecognized_args;
	
	do
	{
		cout << appPrompt() << flush;
		getline(cin, cmdLine);
		
		try
		{
			vmap.clear();
			args = boost_po::split_unix(cmdLine);
			boost_po::parsed_options parsed(
				boost_po::command_line_parser(args)
					.options(cmd_opts["base_opts"])
					.allow_unregistered()
					.run()
			);
			
			boost_po::store(parsed, vmap);
			boost_po::notify(vmap);
			
			unrecognized_args = boost_po::collect_unrecognized(
				parsed.options, boost_po::include_positional
			);
			
			if(vmap.count("help"))
			{
				cout << cmd_opts["base_opts"];
			}
			else if(vmap.count("action"))
			{
				string action = vmap["action"].as<string>();
				
				if(	action == "createGroup" || action == "deleteGroup" || 
					action == "joinGroup" || action == "leaveGroup")
				{
					parseOptions(vmap, unrecognized_args, "group_opts");
					string name = vmap["group"].as<string>();
					
					if(action == "createGroup")
						createGroup(name);
					else if(action == "deleteGroup")
						deleteGroup(name);
					else if(action == "joinGroup")
						joinGroup(name);
					else
						leaveGroup(name);
				}
				else if(action == "sendMessage")
				{
					parseOptions(vmap, unrecognized_args, "gmsg_opts");
					string message = vmap["message"].as<string>();
					string group = vmap["group"].as<string>();
					sendMessage(message, group);
				}
				else if(action == "sendPrivateMessage")
				{
					parseOptions(vmap, unrecognized_args, "pmsg_opts");
					string message = vmap["message"].as<string>();
					string user = vmap["user"].as<string>();
					sendPMessage(message, user);
				}
				else if(action == "getGroupList")
					groupList();
				else if(action == "getUserList")
					userList();
			}
		}
		catch(const boost_po::error& e)
		{
			cout << e.what() << endl;
			continue;
		}
		catch(const Ice::Exception& e)
		{
			cout << e.what() << endl;
			continue;
		}
	}
	while(!vmap.count("quit"));
}

bool
ChatClient::isMyOwnGroup(const string& name)
{
	MapGroup::const_iterator pos = _myGroups.find(name);
	return pos != _myGroups.end() && pos->second == owner;
}

void
ChatClient::parseOptions(boost_po::variables_map& vmap, 
						 const std::vector<string>& args, const string& tag)
{
	boost_po::store(
		boost_po::command_line_parser(args)
			.options(cmd_opts[tag])
			.run(),
		vmap
	);
	boost_po::notify(vmap);
}