
BINARY = imgtest

CXXSOURCES = main.cpp msaImage.cpp

OBJECTS = ${CXXSOURCES:.cpp=.o} ${CSOURCES:.c=.o} 

INCLUDES = -I . 

LOCATIONS = 

LIBRARIES = 

CXXFLAGS = -ggdb -fmessage-length=0 -ansi -Wall
CFLAGS = -ggdb -fmessage-length=0 -ansi -Wall
CXX = g++ 
CC = gcc 

.SUFFIXES:      .cpp .o

.cpp.o:
		@echo
		@echo Building $@		
		${CXX} ${CXXFLAGS} ${INCLUDES} -c -o $@ $<
.c.o:
		@echo
		@echo Building $@		
		${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

all:            ${OBJECTS} ${BINARY} 

${BINARY}:      ${OBJECTS}
		@echo
		@echo Building ${BINARY} Executable
		${CXX} -o $@ \
		${OBJECTS}  \
		${LIBRARIES} \
		${LOCATIONS}
                         
clean:
		rm -f ${BINARY} *.o 



