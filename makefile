install:
	cp -r ./include/ss/ /usr/include/

test:
	cd test && $(MAKE) -j4 && $(MAKE) test && $(MAKE) clean

.PHONY: install test
