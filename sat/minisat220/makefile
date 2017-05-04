#-----------------------------------------------------------------------#
#- GLOBAL DEFS ---------------------------------------------------------#
#-----------------------------------------------------------------------#

# Keep this as generic as possible.

NAME=minisat

# Avoid hardcoding the version number in this file.  This allows us to just
# drop a new distribution package into this directory without changing
# anything else. For other more complicated set-ups / solvers something like
# 'VERSION=960' should also work, if you have the appropriate tar file
# available, e.g., 'picosat-960.tar.gz', which extracts into 'picosat-960'.

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

CXX=g++
CXXFLAGS=-Wall -DNDEBUG -O3

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
	@# apply patch for set_terminate and learn_callback
	@#
	patch minisat-220/minisat/core/Solver.cc < minisat-cc.patch
	patch minisat-220/minisat/core/Solver.h < minisat-h.patch
	@#
	@# patch verbose output
	@#
	sed -i -e 's,"\([=|]\),"c [${SIG}] \1,' $(DIR)/minisat/core/Main.cc
	sed -i -e 's,"\([=|]\),"c [${SIG}] \1,' $(DIR)/minisat/core/Solver.cc
	@#
	@# configure and build library
	@#
	make -C $(DIR) lr
	@#
	@# compile glue code
	@#
	make ipasir$(NAME)glue.o
	@#
	@# merge library and glue code into target
	@#
	cp $(DIR)/build/release/lib/lib$(NAME).a $(TARGET)
	ar r $(TARGET) ipasir$(NAME)glue.o

#-----------------------------------------------------------------------#
#- LOCAL GLUE RULES ----------------------------------------------------#
#-----------------------------------------------------------------------#

ipasir$(NAME)glue.o: ipasir$(NAME)glue.cc ipasir.h makefile
	$(CXX) $(CXXFLAGS) \
	  -DVERSION=\"$(VERSION)\" \
	  -I$(DIR) -I$(DIR)/minisat/core -c ipasir$(NAME)glue.cc

#-----------------------------------------------------------------------#

.FORCE:
.PHONY: all clean
