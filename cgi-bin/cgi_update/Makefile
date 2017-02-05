TARGET	:= update.cgi
all : $(TARGET)
	
CFLAGS	+= -O3

OBJS   := $(patsubst %.c,%.o,$(wildcard *.c))

DEPS    := $(OBJS:.o=.d)

INC_FLAGS += -I. 

LD_FLAGS += -L/usr/local/lib
LD_FLAGS += -lm

%.o : %.c
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@
%.d : %.c
	@set -e; $(CC) $(CFLAGS) $(INC_FLAGS) $(CPPFLAGS) -MM $< | sed -e 's/$(basename $@).o/$(basename $@).o $(basename $@).d/' > $@

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LD_FLAGS) 

clean :
	$(RM) $(OBJS) $(DEPS) $(TARGET)

sinclude $(DEPS)
