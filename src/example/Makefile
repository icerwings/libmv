

PRG = mq_server producer consumer

CC = g++
CXXFLAG = -g -fPIC -Wall -Wno-reorder -std=c++11 -lpthread
SRC = $(wildcard ../core/*.cpp)
OBJ = $(SRC:%.cpp=%.o)

INC=-I. -I../core

all : $(PRG)

mq_server : mq_server.o $(OBJ)
	$(CC) $(CXXFLAG) -o $@ $^ $(LIB) $(INC)

producer : mq_producer.o $(OBJ)
	$(CC) $(CXXFLAG) -o $@ $^ $(LIB) $(INC)

consumer : mq_consumer.o $(OBJ)
	$(CC) $(CXXFLAG) -o $@ $^ $(LIB) $(INC)

%.o : %.cpp
	$(CC) $(CXXFLAG) -c $< -o $@ $(INC)

clean :
	rm $(OBJ) *.o $(PRG)

