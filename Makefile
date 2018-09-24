.PHONY: all clean

all:
	$(MAKE) -C test
	$(MAKE) -C pcart

clean:
	$(MAKE) -C test clean
	$(MAKE) -C pcart clean
