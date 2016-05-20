#####################################################################

# Installer for raw2root
# Cory R. Thornsberry
# May 19th, 2016

#####################################################################

COMPILER = g++

CFLAGS = -Wall -O3 -Iinclude
RFLAGS = -Wall -O3 `root-config --cflags` -Iinclude
LDLIBS = `root-config --libs`
ROOT_INC = `root-config --incdir`

TOP_LEVEL = $(shell pwd)
SOURCE_DIR = $(shell pwd)/source

INSTALL_DIR = ~/bin

# Main executable
MAIN_SRC = $(SOURCE_DIR)/raw2root.cpp

EXECUTABLE = raw2root

########################################################################

$(EXECUTABLE): $(MAIN_SRC)
#	Link the executable
	$(COMPILER) $(RFLAGS) $(MAIN_SRC) -o $@ $(LDLIBS)
	@echo " Done making "$(EXECUTABLE)

########################################################################

.PHONY: install uninstall clean

#####################################################################

install:
#	Uninstall executable to install directory.
	@echo "Installing to "$(INSTALL_DIR)
	@ln -s -f $(TOP_LEVEL)/$(EXECUTABLE) $(INSTALL_DIR)/raw2root

#####################################################################

uninstall:
#	Uninstall executable from install directory.
	@echo "Uninstalling from "$(INSTALL_DIR)
	@rm -f $(INSTALL_DIR)/raw2root

#####################################################################

clean: 
	@rm -f $(EXECUTABLE)
