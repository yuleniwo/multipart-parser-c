CC		:= gcc
CFLAGS	:= -D NDEBUG -Wall -O2 -Wno-unused
LD		:= gcc
LDFLAGS	:= 

SRCS	:= multipart_parser.c main.c
OBJS	:= $(SRCS:%.c=%.o)
TARGET	:= main

DEPS		:= $(OBJS:%.o=%.d)
EXISTS_DEPS	:= $(wildcard $(DEPS))

all: $(DEPS) $(TARGET)
	@echo done.

$(DEPS): %.d:%.c
	$(CC) -c $(CFLAGS) -Wp,-MMD,$@ -MT $(@:%.d=%.o) -MT $@ -o $(@:%.d=%.o) $<

$(TARGET): $(OBJS)
	@echo "---- Build : $@ ----"
	$(LD) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o
	rm -f *.d
	rm -f $(TARGET)

cleanobj:
	rm -f *.o
	rm -f *.d

ifeq ($(findstring $(MAKECMDGOALS), clean cleanobj),)
ifneq ($(EXISTS_DEPS),)
sinclude $(EXISTS_DEPS)
endif
endif

