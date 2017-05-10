CLIENT		= client
SERVER		= server
MANAGER		= manager

SLICE_SRCS	= Chat.cpp \
		  Chat.h

SLICE_OBJS	= Chat.o \

COBJS		= $(SLICE_OBJS) \
		  Client.o \
		  ChatClient.o \
		  ChatUtils.o

SOBJS		= $(SLICE_OBJS) \
		  ChatI.o \
		  Server.o \
		  
MOBJS		= $(SLICE_OBJS) \
		  ChatI.o \
		  Manager.o
		  
CPP		= g++
CPPFLAGS	= -I. -Wall -pedantic
LIBS		= -lIce -lIceUtil -lpthread

# Required packages: libboost-dev libboost-program-options-dev
BOOST_LIBS	= -lboost_program_options

.PHONY: all clean
all: $(CLIENT) $(SERVER) $(MANAGER)

$(SLICE_SRCS):
	slice2cpp Chat.ice

$(CLIENT): $(COBJS)
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LIBS) $(BOOST_LIBS)
	
$(SERVER): $(SOBJS)
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LIBS)
	
$(MANAGER): $(MOBJS)
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf $(COBJS) $(SOBJS) $(MOBJS) $(SLICE_SRCS) $(CLIENT) $(SERVER) $(MANAGER)