CXX ?= g++
ARCHFLAG ?=
CFLAGS ?= -std=c++17 -Wall -march=native -O2 $(ARCHFLAG) -I..
LDFLAGS ?= -static
SRCS := $(shell find . -name '*.cpp')
OBJS := $(SRCS:%.cpp=%.o)
DEPS := $(SRCS:%.cpp=%.d)

.PHONY: clean all FORCE

all: pcartcli

pcartcli: $(OBJS) ../pcart/pcart.a
	$(CXX) $(CFLAGS) $(OBJS) ../pcart/pcart.a -o pcartcli $(LDFLAGS)

../pcart/pcart.a: FORCE
	$(MAKE) -C ../pcart

FORCE:

%.o: %.cpp
	$(CXX) $(CFLAGS) -MMD -c $< -o $@

clean:
	rm -f pcartcli $(OBJS) $(DEPS)

-include $(DEPS)
