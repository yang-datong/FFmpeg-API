CC:=g++
BIN:=a.out
CPPFLAGS:=-g -Wall -std=c++11 -I static/include
LIBS=-L static/lib/ -lavfilter -lavformat -lavcodec -lavutil -lswscale -lz -lx264
SRC:=$(wildcard *.cpp)
OBJS:=$(patsubst %.cpp,%.o,$(SRC))

$(BIN):$(OBJS)
	$(CC) $^ -o $@ $(LIBS)

clean:
	rm $(BIN) *.o output.* divide.* *.yuv *.h264
