#--------------------------------------------------------------------------#
# The target name should be the name of this app, which actually should be
# the same as the name of this directory, e.g., 'genipasat' etc.
#--------------------------------------------------------------------------#

TARGET=$(shell basename "`pwd`")

#--------------------------------------------------------------------------#
# When called from the 'mkone.sh' script the 'IPASIRSOLVER' variable will be
# overwritten.  For testing purposes we simply set it to the default PicoSAT
# front-end (currently 'picosat960').  This allows to call 'make' in this
# directory without the need to specify the 'IPASIRSOLVER' variable.
#--------------------------------------------------------------------------#

IPASIRSOLVER	?= picosat961

#--------------------------------------------------------------------------#
# There is usually no need to change something here unless you want to force
# a specific compiler or specific compile flags.
#--------------------------------------------------------------------------#

CC	?=	gcc
CXX	?=	g++
CFLAGS	?=	-Wall -DNDEBUG -O3

DEPS	=	../../sat/$(IPASIRSOLVER)/libipasir$(IPASIRSOLVER).a

LIBS	=	-L../../sat/$(IPASIRSOLVER)/ -lipasir$(IPASIRSOLVER)
LIBS	+=	$(shell cat ../../sat/$(IPASIRSOLVER)/LIBS 2>/dev/null)

LINK	=	$(shell	if [ -f ../../sat/$(IPASIRSOLVER)/LINK ]; \
			then \
			  cat ../../sat/$(IPASIRSOLVER)/LINK; \
			else \
			  echo $(CXX) $(CFLAGS); \
			fi)

#--------------------------------------------------------------------------#
# Here comes the real makefile part which needs to be adapted and provide
# both an 'all' and a 'clean' target.  In essence you need to provide
# linking options, which links your app to a generic 'IPASIRSOLVER'.
#--------------------------------------------------------------------------#

# This part is still generic and the specific part comes further down.

all: $(TARGET)

clean:
	rm -f $(TARGET) *.o *~


#--------------------------------------------------------------------------#
# Some back-end SAT solvers require C++ linking, e.g., 'g++'.  If your app
# is using C++ you might want to explicitly set (and comment out)
#
#   LINK=g++
#
# or something similar.

$(TARGET): $(TARGET).o $(DEPS)
	${LINK} -o $@ $(TARGET).o $(LIBS)

#--------------------------------------------------------------------------#
# Local app specific rules.
#--------------------------------------------------------------------------#

$(TARGET).o: $(TARGET).cc ipasir.h makefile
	$(CXX) $(CFLAGS) -c $(TARGET).cc
