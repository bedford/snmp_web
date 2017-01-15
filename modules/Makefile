TARGET := sms_test

all : $(TARGET)

CFLAGS	:= -g -Wall
#LD_FLAGS:= -lpthread
CC	:= gcc
#CC	:= arm-linux-gcc

OBJS   := $(patsubst %.c,%.o,$(wildcard *.c))

%.o : %.c
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@
%.d : %.c
	@set -e; $(CC) $(CFLAGS) $(INC_FLAGS) $(CPPFLAGS) -MM $< | sed -e 's/$(basename $@).o/$(basename $@).o $(basename $@).d/' > $@

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LD_FLAGS)

clean :
	$(RM) $(OBJS) $(DEPS) $(TARGET) $(TARGET_LIB)
