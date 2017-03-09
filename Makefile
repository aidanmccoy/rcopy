all: rcopy

run:
	clear
	./rcopy localTest remoteTest 1 2 3.5 208.94.61.49 8080

rcopy: rcopy.c rcopy.h
	clear
	@echo "Compiling rcopy"
	gcc -o rcopy rcopy.c

clean:
	clear
	rm rcopy
	
