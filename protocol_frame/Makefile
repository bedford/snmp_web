SHARE_TARGET = libprotocol.so
TEST_TARGET = protocol_test

CC = arm-linux-gcc
CFLAGS += -O3 -fPIC

LIB_OBJS := list.o crc16.o ups.o environment_k25.o protocol_interfaces.o
#LIB_OBJS := list.o crc16.o ups.o external_io.o protocol_interfaces.o

INC_FLAGS	+= -I.
LD_FLAGS	+= -L. -lprotocol

TEST_SRC = test.c

.PHONY : all shared test clean

%.o : %.c
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@

all : shared test

shared : $(SHARE_TARGET)

test : $(TEST_TARGET)

$(SHARE_TARGET) : $(LIB_OBJS)
	$(CC) -shared -o $@ $^

$(TEST_TARGET) : $(TEST_SRC)
	$(CC) $(TEST_SRC) -o $@ $(LD_FLAGS) $(INC_FLAGS)

clean :
	$(RM) $(LIB_OBJS) $(SHARE_TARGET) $(TEST_TARGET)

