NAME = cmdrunner
PREFIX = /usr

all:
	$(MAKE) clear
	$(MAKE) release

release:
	cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Release -S . -B ./build -G Ninja
	cmake --build ./build

clear:
	rm -rf build

install.core:
	@if [ ! -f ./build/cmdrunner ]; then echo -en "You need to run $(MAKE) all first.\n" && exit 1; fi
	@echo -en "!NOTE: Please note make install does not compile cmdrunner and only installs the already built files."
	mkdir -p ${PREFIX}/bin
	cp -f ./build/cmdrunner ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/cmdrunner

install: install.core

uninstall:
	rm -f ${PREFIX}/bin/cmdrunner
