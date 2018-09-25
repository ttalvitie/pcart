CXX ?= g++
CFLAGS ?= -std=c++17 -Wall -march=native -I..
LDFLAGS ?= 
SRCS := $(shell find . -name '*.cpp')
OBJS := $(SRCS:%.cpp=%.o)
DEPS := $(SRCS:%.cpp=%.d)

.PHONY: clean all FORCE

all: example

example: $(OBJS) ../pcart/pcart.a
	$(CXX) $(CFLAGS) $(OBJS) ../pcart/pcart.a -o example $(LDFLAGS)

../pcart/pcart.a: FORCE
	$(MAKE) -C ../pcart

FORCE:

%.o: %.cpp
	$(CXX) $(CFLAGS) -MMD -c $< -o $@

clean:
	rm -f example $(OBJS) $(DEPS)

-include $(DEPS)
