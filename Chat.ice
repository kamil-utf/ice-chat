module Chat
{
	interface ChatCallback;
	interface ChatUser;
	interface ChatGroup;
	interface ChatGroupManager;
	interface ChatServer;
	
	exception NameDoesNotExist {};
	exception NameAlreadyExists {};
	exception UserDoesNotExist {};
	exception UserAlreadyExists {};
	exception ManagerDoesNotExist {};
	exception ManagerAlreadyExists {};
	
	sequence<ChatUser*> Users;
	sequence<ChatGroup*> Groups;
	
	interface ChatCallback
	{
		void receive(string message);
	};
	
	interface ChatUser
	{
		string name();
		
		void setCallback(ChatCallback* callback);
		void sendPMessage(string message, ChatUser* sender);
		void sendMessage(string message, ChatUser* sender, ChatGroup *group);
		void destroy();
	};
	
	interface ChatGroup
	{
		string name();
		
		Users getUserList();
		void join(ChatUser *who) throws UserAlreadyExists;
		void leave(ChatUser *who) throws UserDoesNotExist;
		void sendMessage(string message, ChatUser* sender);
	};
	
	interface ChatGroupManager
	{
		Groups getGroupList();
		ChatGroup* findGroupByName(string name);
		ChatGroup* createGroup(string name) throws NameAlreadyExists;
		void deleteGroup(string name) throws NameDoesNotExist;
	};
	
	interface ChatServer
	{
		Users getUserList();
		ChatUser* findUserByName(string name) throws NameDoesNotExist;
		ChatUser* login(string name) throws NameAlreadyExists;
		
		Groups getGroupList();
		ChatGroup* findGroupByName(string name) throws NameDoesNotExist;
		void createGroup(string name) throws NameAlreadyExists;
		void deleteGroup(string name) throws NameDoesNotExist;
		
		void addGroupManager(ChatGroupManager* manager) throws ManagerAlreadyExists;
		void removeGroupManager(ChatGroupManager* manager) throws ManagerDoesNotExist;
	};
};