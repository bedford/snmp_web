TARGET	:= common.cgi
all : $(TARGET)
	
CFLAGS	+= -O3

OBJS   := $(patsubst %.c,%.o,$(wildcard *.c))

DEPS    := $(OBJS:.o=.d)

INC_FLAGS += -I.

LD_FLAGS += -lm

CC = arm-linux-gcc
LD_FLAGS += -L/opt/work/web/cgi-bin/cgi_common/arm-lib -lsqlite3
#LD_FLAGS += -L/opt/work/web/cgi-bin/cgi_common/x86-lib -lsqlite3

%.o : %.c
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@
%.d : %.c
	@set -e; $(CC) $(CFLAGS) $(INC_FLAGS) $(CPPFLAGS) -MM $< | sed -e 's/$(basename $@).o/$(basename $@).o $(basename $@).d/' > $@

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LD_FLAGS) 

clean :
	$(RM) $(OBJS) $(DEPS) $(TARGET)

sinclude $(DEPS)
