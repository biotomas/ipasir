#-----------------------------------------------------------------------#
#- GLOBAL DEFS ---------------------------------------------------------#
#-----------------------------------------------------------------------#

# Keep this as generic as possible.

NAME=picosat

# Avoid hardcoding the version number in this file.  This allows us to just
# drop a new distribution package into this directory without changing
# anything else. For other more complicated set-ups / solvers something like
# 'VERSION=961' should also work, if you have the appropriate tar file
# available, e.g., 'picosat-961.tar.gz', which extracts into 'picosat-961'.

# NOTE: requires GNU make extension '$(shell ...)'.

VERSION=$(shell ls $(NAME)-*.tar.gz | tail -1 | \
		sed -e 's,$(NAME)-,,' -e 's,.tar.gz,,')

#-----------------------------------------------------------------------#
# Solver signatures have to be both valid file names and C symbols.
# Since Picosat uses a dash '-' for the package name, we have to
# differentiate between with (directory / package name) and without
# dash (the signature).

SIG=$(NAME)$(VERSION)
DIR=$(NAME)-$(VERSION)
TARGET=libipasir$(SIG).a

#-----------------------------------------------------------------------#

CC=gcc
CFLAGS=-Wall -DNDEBUG -O3 -fPIC

#-----------------------------------------------------------------------#
#- REQUIRED TOP RULES --------------------------------------------------#
#-----------------------------------------------------------------------#

all: $(TARGET)

clean:
	rm -rf $(DIR)
	rm -f *.o *.a

#-----------------------------------------------------------------------#
#- INVISIBLE INTERNAL SUB RULES ----------------------------------------#
#-----------------------------------------------------------------------#

libipasir$(SIG).a: .FORCE
	@#
	@# extract library
	@#
	rm -rf $(DIR)
	tar xvf $(DIR).tar.gz
	@#
	@# configure and build library
	@#
	cd $(DIR); ./configure -O --no-trace --shared
	make -C $(DIR)
	@#
	@# compile glue code
	@#
	make ipasir$(NAME)glue.o
	@#
	@# merge library and glue code into target
	@#
	cp $(DIR)/lib$(NAME).a $(TARGET)
	ar r $(TARGET) ipasir$(NAME)glue.o

#-----------------------------------------------------------------------#
#- LOCAL GLUE RULES ----------------------------------------------------#
#-----------------------------------------------------------------------#

ipasir$(NAME)glue.o: ipasir$(NAME)glue.c ipasir.h makefile
	$(CC) $(CFLAGS) \
	  -DVERSION=\"$(VERSION)\" \
	  -I$(DIR) -c ipasir$(NAME)glue.c

#-----------------------------------------------------------------------#

.FORCE:
.PHONY: all clean
