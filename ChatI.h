#ifndef __ChatI_h__
#define __ChatI_h__

#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>
#include <Chat.h>
#include <vector>
#include <map>

namespace Chat
{
////////// Forward declaration ChatServerI ////////////////////

class ChatServerI;
typedef IceUtil::Handle<ChatServerI> ChatServerIPtr;

////////// ChatObject ////////////////////
	
class ChatObject
{
public:
	ChatObject();
	
	Ice::Identity ident() const;
private:
	Ice::Identity _ident;
};

////////// ChatUserI ////////////////////
	
class ChatUserI : public ChatObject, virtual public ChatUser
{
public:
	ChatUserI(const std::string&,
			  const Chat::ChatServerIPtr&);
	
	virtual std::string name(const Ice::Current&);
	
	
	virtual void setCallback(const Chat::ChatCallbackPrx&,
							 const Ice::Current&);
	
	virtual void sendPMessage(const std::string&,
							  const Chat::ChatUserPrx&,
							  const Ice::Current&);
	
	virtual void sendMessage(const std::string&,
							 const Chat::ChatUserPrx&,
							 const Chat::ChatGroupPrx&,
							 const Ice::Current&);
	
	virtual void destroy(const Ice::Current&);
private:
	IceUtil::Mutex _mutex;
	const std::string _name;
	const ChatServerIPtr _host;
	Chat::ChatCallbackPrx _callback;
	bool _destroyed;
};

typedef IceUtil::Handle<ChatUserI> ChatUserIPtr;

////////// ChatGroupI ////////////////////

class ChatGroupI : public ChatObject, virtual public ChatGroup
{
public:
	ChatGroupI(const std::string&);
	
	virtual std::string name(const Ice::Current&);
	
	virtual Chat::Users getUserList(const Ice::Current&);
	
	virtual void join(const Chat::ChatUserPrx&,
					  const Ice::Current&);
	
	virtual void leave(const Chat::ChatUserPrx&,
					   const Ice::Current&);
	
	virtual void sendMessage(const std::string&,
							 const Chat::ChatUserPrx&,
							 const Ice::Current&);
private:
	Chat::Users::iterator findMemberByIdent(const Ice::Identity&);
	
	IceUtil::Mutex _mutex;
	const std::string _name;
	Chat::Users _users;
};

typedef IceUtil::Handle<ChatGroupI> ChatGroupIPtr;

////////// ChatGroupManagerI ////////////////////

class ChatGroupManagerI : public ChatObject, virtual public ChatGroupManager
{
public:
	virtual Chat::Groups getGroupList(const Ice::Current&);
	
	virtual Chat::ChatGroupPrx findGroupByName(const std::string&,
											   const Ice::Current&);
	
	virtual Chat::ChatGroupPrx createGroup(const std::string&,
										   const Ice::Current&);
	
	virtual void deleteGroup(const std::string&,
							 const Ice::Current&);
private:
	typedef std::map<std::string, Chat::ChatGroupIPtr> group_map;
	
	IceUtil::Mutex _mutex;
	group_map _groups;
};

typedef IceUtil::Handle<ChatGroupManagerI> ChatGroupManagerIPtr;

////////// ChatServerI ////////////////////

class ChatServerI : virtual public ChatServer
{
public:
	ChatServerI();
	
	virtual Chat::Users getUserList(const Ice::Current&);
	
	virtual Chat::ChatUserPrx findUserByName(const std::string&,
											 const Ice::Current&);
	
	virtual Chat::ChatUserPrx login(const std::string&,
									const Ice::Current&);
	
	void logout(const std::string&);
	
	virtual Chat::Groups getGroupList(const Ice::Current&);
	
	virtual Chat::ChatGroupPrx findGroupByName(const std::string&,
											   const Ice::Current&);
	
	virtual void createGroup(const std::string&,
							 const Ice::Current&);
	
	virtual void deleteGroup(const std::string&,
							 const Ice::Current&);
	
	virtual void addGroupManager(const Chat::ChatGroupManagerPrx&,
								 const Ice::Current&);
	
	virtual void removeGroupManager(const Chat::ChatGroupManagerPrx&,
									const Ice::Current&);
private:
	typedef std::map<std::string, Chat::ChatUserIPtr> user_map;
	typedef std::vector<Chat::ChatGroupManagerPrx> manager_vector;
	
	manager_vector::iterator findManagerByIdent(const Ice::Identity&);
	Chat::Groups::iterator findCrowdByName(const std::string&);
	
	IceUtil::Mutex _mutex;
	manager_vector _managers;
	user_map _users;
	Chat::Groups _groups;
};

typedef IceUtil::Handle<ChatServerI> ChatServerIPtr;
}

#endif
