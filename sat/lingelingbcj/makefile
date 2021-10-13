#-----------------------------------------------------------------------#
#- GLOBAL DEFS ---------------------------------------------------------#
#-----------------------------------------------------------------------#

# Keep this as generic as possible.

NAME=lingeling
VERSION=bcj

#-----------------------------------------------------------------------#
# Solver signatures have to be both valid file names and C symbols.
# Since Picosat uses a dash '-' for the package name, we have to
# differentiate between with (directory / package name) and without
# dash (the signature).

SIG=$(NAME)$(VERSION)
DIR=$(NAME)-$(VERSION)
TARGET=libipasir$(SIG).a

#-----------------------------------------------------------------------#

CC=g++
CFLAGS=-Wall -DNDEBUG -O3 -fPIC

#-----------------------------------------------------------------------#
#- REQUIRED TOP RULES --------------------------------------------------#
#-----------------------------------------------------------------------#

all: $(TARGET)

clean:
	rm -rf $(DIR)
	rm -f *.o *.a *.tar.gz

#-----------------------------------------------------------------------#
#- INVISIBLE INTERNAL SUB RULES ----------------------------------------#
#-----------------------------------------------------------------------#

libipasir$(SIG).a: .FORCE
	@#
	@# get and extract library
	@#
	rm -rf $(DIR) rm *.tar.gz
	wget "http://fmv.jku.at/lingeling/lingeling-bcj-78ebb86-180517.tar.gz"
	tar xvf lingeling-bcj-78ebb86-180517.tar.gz
	mv lingeling-bcj-78ebb86-180517 $(DIR)
	@#
	@# configure and build library
	@#
	cd $(DIR); ./configure.sh -fPIC
	make liblgl.a -C $(DIR)
	@#
	@# compile glue code
	@#
	make ipasir$(NAME)glue.o
	@#
	@# merge library and glue code into target
	@#
	cp $(DIR)/liblgl.a $(TARGET)
	ar r $(TARGET) ipasir$(NAME)glue.o

#-----------------------------------------------------------------------#
#- LOCAL GLUE RULES ----------------------------------------------------#
#-----------------------------------------------------------------------#

ipasir$(NAME)glue.o: ipasir$(NAME)glue.cpp ipasir.h makefile
	$(CC) $(CFLAGS) \
	  -DVERSION=\"$(VERSION)\" \
	  -I$(DIR) -c ipasir$(NAME)glue.cpp

#-----------------------------------------------------------------------#

.FORCE:
.PHONY: all clean
