SHARE_TARGET = libprotocol.so
TEST_TARGET = protocol_test

CFLAGS += -O3

LIB_OBJS := list.o ups.o protocol_interfaces.o

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

