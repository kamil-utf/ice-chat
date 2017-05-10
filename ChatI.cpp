#include <ChatI.h>
#include <time.h>

using namespace std;
using namespace Chat;

////////// ChatObject ////////////////////

ChatObject::ChatObject()
{
	_ident.name = IceUtil::generateUUID();
}

Ice::Identity
ChatObject::ident() const
{
	return _ident;
}

////////// ChatUserI ////////////////////

ChatUserI::ChatUserI(const string& name, const ChatServerIPtr& host)
	: _name(name), _host(host), _destroyed(false) {}

string
ChatUserI::name(const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	if(_destroyed)
	{
		throw Ice::ObjectNotExistException(__FILE__, __LINE__);
	}
	
	return _name;
}

void
ChatUserI::setCallback(const ChatCallbackPrx& callback,
					   const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	if(_destroyed)
	{
		throw Ice::ObjectNotExistException(__FILE__, __LINE__);
	}
	
	if(_callback || !callback)
	{
		return;
	}
	
	_callback = callback;
}

void
ChatUserI::sendPMessage(const string& message,
						const ChatUserPrx& sender,
						const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	if(_destroyed)
	{
		throw Ice::ObjectNotExistException(__FILE__, __LINE__);
	}
	
	if(!_callback)
	{
		return;
	}
	
	_callback->receive("@" + sender->name() + ": " + message);
}

void
ChatUserI::sendMessage(const string& message,
					   const ChatUserPrx& sender,
					   const ChatGroupPrx& group,
					   const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	if(_destroyed)
	{
		throw Ice::ObjectNotExistException(__FILE__, __LINE__);
	}
	
	if(!_callback)
	{
		return;
	}
	
	_callback->receive(group->name() + " @" + sender->name() + ": " + message);
}

void
ChatUserI::destroy(const Ice::Current& c)
{
	{
		IceUtil::Mutex::Lock lock(_mutex);
		
		if(_destroyed)
		{
			throw Ice::ObjectNotExistException(__FILE__, __LINE__);
		}
		
		c.adapter->remove(ident());
		_destroyed = true;
	}
	
	_host->logout(_name);
}

////////// ChatGroupI ////////////////////

ChatGroupI::ChatGroupI(const string& name): _name(name) {}

string
ChatGroupI::name(const Ice::Current& c)
{
	return _name;
}

Users
ChatGroupI::getUserList(const Ice::Current& c)
{
    IceUtil::Mutex::Lock lock(_mutex);
	return _users;
}

void
ChatGroupI::join(const ChatUserPrx& who,
				 const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	Ice::Identity ident = who->ice_getIdentity();
	Users::iterator pos = findMemberByIdent(ident);
	if(!who || pos != _users.end())
	{
		throw UserAlreadyExists();
	}
	
	_users.push_back(who);
}

void
ChatGroupI::leave(const ChatUserPrx& who,
				  const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	Ice::Identity ident = who->ice_getIdentity();
	Users::iterator pos = findMemberByIdent(ident);
	if(pos == _users.end())
	{
		throw UserDoesNotExist();
	}
	
	_users.erase(pos);
}

void
ChatGroupI::sendMessage(const string& message,
						const ChatUserPrx& sender,
						const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	ChatGroupPrx gr = 
		ChatGroupPrx::uncheckedCast(c.adapter->createProxy(ident()));
	
	Ice::Identity author_ident = sender->ice_getIdentity();
	for(Users::iterator i = _users.begin(); i != _users.end(); ++i)
	{
		ChatUserPrx usr = ChatUserPrx::checkedCast(*i);
		if(usr->ice_getIdentity() != author_ident)
		{
			usr->sendMessage(message, sender, gr);
		}
	}
}

Users::iterator
ChatGroupI::findMemberByIdent(const Ice::Identity& ident)
{
	for(Users::iterator i = _users.begin(); i != _users.end(); ++i)
	{
		ChatUserPrx prx = ChatUserPrx::uncheckedCast(*i);
		if(prx->ice_getIdentity() == ident)
		{
			return i;
		}
	}
	
	return _users.end();
}

////////// ChatGroupManagerI ////////////////////

Groups
ChatGroupManagerI::getGroupList(const Ice::Current& c)
{
    IceUtil::Mutex::Lock lock(_mutex);
	
	Groups list;
	for(group_map::const_iterator i = _groups.begin(); i != _groups.end(); ++i)
	{
		ChatGroupIPtr gr = i->second;
		list.push_back(ChatGroupPrx::uncheckedCast(c.adapter->createProxy(gr->ident())));
	}
	
	return list;
}

ChatGroupPrx
ChatGroupManagerI::findGroupByName(const string& name,
								   const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	group_map::const_iterator pos = _groups.find(name);
	if(pos == _groups.end())
	{
		return (ChatGroupPrx) 0;
	}
	
	ChatGroupIPtr gr = pos->second;
	return ChatGroupPrx::uncheckedCast(c.adapter->createProxy(gr->ident()));
}

ChatGroupPrx
ChatGroupManagerI::createGroup(const string& name,
							   const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	group_map::const_iterator pos = _groups.find(name);
	if(name.empty() || pos != _groups.end())
	{
		throw NameAlreadyExists();
	}
	
	ChatGroupIPtr gr = new ChatGroupI(name);
	Ice::ObjectPrx prx = c.adapter->add(gr, gr->ident());
	_groups[name] = gr;
	return ChatGroupPrx::uncheckedCast(prx);
}

void
ChatGroupManagerI::deleteGroup(const string& name,
							   const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	group_map::iterator pos = _groups.find(name);
	if(pos == _groups.end())
	{
		throw NameDoesNotExist();
	}
	
	ChatGroupIPtr gr = pos->second;
	c.adapter->remove(gr->ident());
	_groups.erase(pos);
}

////////// ChatServerI ////////////////////

ChatServerI::ChatServerI()
{
	srand(time(NULL));
}

Users
ChatServerI::getUserList(const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
    Users list;
	for(user_map::const_iterator i = _users.begin(); i != _users.end(); ++i)
	{
		ChatUserIPtr usr = i->second;
		list.push_back(ChatUserPrx::uncheckedCast(c.adapter->createProxy(usr->ident())));
	}
	
	return list;
}

ChatUserPrx
ChatServerI::findUserByName(const string& name,
							const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	user_map::const_iterator pos = _users.find(name);
	if(pos == _users.end())
	{
		throw NameDoesNotExist();
	}
	
	ChatUserIPtr usr = pos->second;
	return ChatUserPrx::uncheckedCast(c.adapter->createProxy(usr->ident()));
}

ChatUserPrx
ChatServerI::login(const string& name,
				   const Ice::Current& c)
{
    IceUtil::Mutex::Lock lock(_mutex);
	
	user_map::const_iterator pos = _users.find(name);
	if(name.empty() || pos != _users.end())
	{
		throw NameAlreadyExists();
	}
	
	ChatUserIPtr usr = new ChatUserI(name, this);
	Ice::ObjectPrx prx = c.adapter->add(usr, usr->ident());
	_users[name] = usr;
	return ChatUserPrx::uncheckedCast(prx);
}

void
ChatServerI::logout(const string& name)
{
	IceUtil::Mutex::Lock lock(_mutex);

	user_map::iterator pos = _users.find(name);
	if(pos != _users.end())
	{
		_users.erase(pos);
	}
}

Groups
ChatServerI::getGroupList(const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
    return _groups;
}

ChatGroupPrx
ChatServerI::findGroupByName(const string& name,
							 const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	Groups::iterator pos = findCrowdByName(name);
	if(pos == _groups.end())
	{
		throw NameDoesNotExist();
	}
	
	return ChatGroupPrx::uncheckedCast(*pos);
}

void
ChatServerI::createGroup(const string& name,
						 const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	Groups::iterator pos = findCrowdByName(name);
	if(name.empty() || pos != _groups.end())
	{
		throw NameAlreadyExists();
	}
	
	if(!_managers.size())
	{
		throw Ice::UnknownException(__FILE__, __LINE__, "Lack of ChatGroupManager");
	}
	
	int r = rand() % _managers.size();
	ChatGroupPrx gr = _managers[r]->createGroup(name);
	_groups.push_back(gr);
}

void
ChatServerI::deleteGroup(const string& name,
						 const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);
	
	unsigned int i = 0;
	while(!_managers[i]->findGroupByName(name) && i < _managers.size())
		i++;
	
	Groups::iterator pos = findCrowdByName(name);
	if(i >= _managers.size() || pos == _groups.end())
	{
		throw NameDoesNotExist();
	}
	
	_managers[i]->deleteGroup(name);
	_groups.erase(pos);
}

void
ChatServerI::addGroupManager(const ChatGroupManagerPrx& manager,
							 const Ice::Current& c)
{	
	IceUtil::Mutex::Lock lock(_mutex);
	
	Ice::Identity ident = manager->ice_getIdentity();
	manager_vector::iterator pos = findManagerByIdent(ident);
	if(!manager || pos != _managers.end())
	{
		throw ManagerAlreadyExists();
	}
	
	_managers.push_back(manager);
}

void
ChatServerI::removeGroupManager(const ChatGroupManagerPrx& manager,
								const Ice::Current& c)
{
	IceUtil::Mutex::Lock lock(_mutex);

	Ice::Identity ident = manager->ice_getIdentity();
	manager_vector::iterator pos = findManagerByIdent(ident);
	if(pos == _managers.end())
	{
		throw ManagerDoesNotExist();
	}
	
	_managers.erase(pos);
}

ChatServerI::manager_vector::iterator
ChatServerI::findManagerByIdent(const Ice::Identity& ident)
{
	for(manager_vector::iterator i = _managers.begin(); i != _managers.end(); ++i)
	{
		ChatGroupManagerPrx prx = ChatGroupManagerPrx::uncheckedCast(*i);
		if(prx->ice_getIdentity() == ident)
		{
			return i;
		}
	}
	
	return _managers.end();
}

Groups::iterator
ChatServerI::findCrowdByName(const string& name)
{
	for(Groups::iterator i = _groups.begin(); i != _groups.end(); ++i)
	{
		ChatGroupPrx prx = ChatGroupPrx::uncheckedCast(*i);
		if(prx->name() == name)
		{
			return i;
		}
	}
	
	return _groups.end();
}
