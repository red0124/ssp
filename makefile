install:
	cp -r ./include/ss/ /usr/local/include/

uninstall:
	rm -rf /usr/local/include/ss/*.hpp

test:
	cd test && $(MAKE) -j4 && $(MAKE) test && $(MAKE) clean

.PHONY: install test
