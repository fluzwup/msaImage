
BINARY = imgtest

CXXSOURCES = main.cpp msaImage.cpp ColorspaceConversion.cpp msaFilters.cpp

OBJECTS = ${CXXSOURCES:.cpp=.o} ${CSOURCES:.c=.o} 

INCLUDES = -I . 

LOCATIONS = 

LIBRARIES = 

CXXFLAGS = -ggdb -Wall
CFLAGS = -ggdb -Wall
CXX = g++ --std=c++11
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



