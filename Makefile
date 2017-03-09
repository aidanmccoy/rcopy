all: rcopy server

run:
	clear
	./rcopy localTest remoteTest 1 2 3.5 208.94.61.49 8080
	@echo "RUNNING SERVER"
	./server 3.5 4040

rcopy: rcopy.c rcopy.h
	clear
	@echo "Compiling rcopy"
	gcc -o rcopy rcopy.c

server: server.c server.h
	@echo "COMPILING SERVER"
	gcc -o server server.c

clean:
	clear
	rm rcopy
	
