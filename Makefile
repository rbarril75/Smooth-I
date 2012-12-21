EXEC = smooth

DEPS = smooth.o

COMPILER = g++
COMPILER_OPTS = -c -g -Wall

LINKER = g++
LINKER_OPTS = -framework GLUT -framework OpenGL -framework Carbon

all : $(EXEC)

$(EXEC) : $(DEPS)
	$(LINKER) -o $(EXEC) $(LINKER_OPTS) $(DEPS)

mp4.o : mp4.cpp
	$(COMPILER) $(COMPILER_OPTS) smooth.cpp

clean : 
	-rm *.o $(EXEC)
