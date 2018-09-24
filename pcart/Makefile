CXX ?= g++
CFLAGS ?= -std=c++17 -Wall -march=native -I..
SRCS := $(shell find . -name '*.cpp')
OBJS := $(SRCS:%.cpp=%.o)
DEPS := $(SRCS:%.cpp=%.d)

.PHONY: clean all

all: pcart.a

pcart.a: $(OBJS)
	rm -f pcart.a
	ar rs pcart.a $(OBJS)

%.o: %.cpp
	$(CXX) $(CFLAGS) -MMD -c $< -o $@

clean:
	rm -f pcart.a $(OBJS) $(DEPS)

-include $(DEPS)
