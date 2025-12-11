all: compile clean

compile:
	g++ -c Config.cpp 
	g++ -c Evolution.cpp 
	g++ -c Robot.cpp 
	g++ -c Utils.cpp 
	g++ -c main.cpp
	g++ -o main main.o Config.o Robot.o Evolution.o Utils.o 

clean:
	rm *.o

run:
	./main
