CXX ?= g++
ARCHFLAG ?=
CFLAGS ?= -std=c++17 -Wall -march=native -O2 $(ARCHFLAG) -I..
LDFLAGS ?=
SRCS := $(shell find . -name '*.cpp')
OBJS := $(SRCS:%.cpp=%.o)
DEPS := $(SRCS:%.cpp=%.d)

.PHONY: clean all FORCE

all: test

test: $(OBJS) ../pcart/pcart.a
	$(CXX) $(CFLAGS) $(OBJS) ../pcart/pcart.a -o test $(LDFLAGS)

../pcart/pcart.a: FORCE
	$(MAKE) -C ../pcart

FORCE:

%.o: %.cpp
	$(CXX) $(CFLAGS) -MMD -c $< -o $@

clean:
	rm -f test $(OBJS) $(DEPS)

-include $(DEPS)
