LIBS = -pthread
FLAGS = -Wall -Werror -std=c++11


src = $(wildcard *.cpp)
objects = $(patsubst %.cpp, %.o, $(src))



FindGoogle: $(objects)
	g++  $(objects) $(LIBS) $(FLAGS) -o $@


$(objects):%.o : %.cpp 
	g++ -c $(FLAGS)  $<  -o $@


.PHONY:clean
clean:
	-rm -f *.o




