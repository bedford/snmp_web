TARGET	:= snmp_app
all : $(TARGET)
	
CFLAGS	+= -O3

OBJS   := $(patsubst %.c,%.o,$(wildcard *.c))

DEPS    := $(OBJS:.o=.d)

INC_FLAGS += -I.
INC_FLAGS += -I/usr/local/ssl/include

LD_FLAGS += -lpthread
#LD_FLAGS += -L./libs/x86 -lprotocol -lssl -lcrypto
LD_FLAGS += -L./libs -lprotocol -lssl -lcrypto

CC = arm-linux-gcc
LD_FLAGS += -L/opt/work/web/cgi-bin/cgi_common/arm-lib -lsqlite3
#LD_FLAGS += -L/opt/work/web/cgi-bin/cgi_common/x86-lib -lsqlite3

ROOT_PATH := $(shell pwd)
APP_VRESION_STRING := $(shell ./setlocalversion.sh $(ROOT_PATH))

%.o : %.c
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@
%.d : %.c
	@set -e; $(CC) $(CFLAGS) $(INC_FLAGS) $(CPPFLAGS) -MM $< | sed -e 's/$(basename $@).o/$(basename $@).o $(basename $@).d/' > $@

$(TARGET) : $(OBJS)
	$(shell cat version.h.template | sed "s/app_version/\"$(APP_VRESION_STRING)\"/g" > version.h)
	$(CC) $(CFLAGS) -o $@ $^ $(LD_FLAGS) 

clean :
	$(RM) $(OBJS) $(DEPS) $(TARGET)

sinclude $(DEPS)
