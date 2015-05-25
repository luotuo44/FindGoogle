LIBS = -pthread
FLAGS = -Wall -Werror -std=c++11

object = ConnectPort.o DNS_Machine.o SocketOps.o DNS.o IPQueue.o \
	 Reactor.o 


FindGoogle:main.cpp $(object)
	g++ $< $(object) $(LIBS) $(FLAGS) -o $@


$(object):%.o : %.cpp 
	g++ -c $(FLAGS)  $<  -o $@


.PHONY:clean
clean:
	-rm -f *.o




