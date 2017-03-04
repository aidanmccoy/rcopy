all: rcopy

rcopy:
	clear
	@echo "Compiling rcopy"
	gcc -o rcopy rcopy.c

clean:
	clear
	rm rcopy
	
