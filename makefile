FLAGS = -Wall -Werror -std=c++11

src = $(wildcard *.cpp)
objects = $(patsubst %.cpp, %.o, $(src))

TARGET=FindGoogle

$(TARGET):$(objects)
	g++ $(FLAGS) `pkg-config --libs libevent` $^ -o $@


$(objects):%.o : %.cpp 
	g++ -c $(FLAGS) `pkg-config --cflags libevent`  $<  -o $@



.PHONY:clean
clean:
	rm -rf $(TARGET) $(objects)




