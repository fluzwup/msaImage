
BINARY = imgtest

CXXSOURCES = main.cpp msaImage.cpp ColorspaceConversion.cpp msaFilters.cpp pngIO.cpp bmpIO.cpp\
	     msaAnalysis.cpp msaAffine.cpp 
CSOURCES = 

OBJECTS = ${CXXSOURCES:.cpp=.o} ${CSOURCES:.c=.o} 

INCLUDES = -I . 

LOCATIONS = 

LIBRARIES = -lpng 

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
		${LOCATIONS} \
		${LIBRARIES} 
                         
clean:
		rm -f ${BINARY} *.o 



