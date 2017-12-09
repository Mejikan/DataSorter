all: sorter.o sortFile.o
	gcc -Wall -Werror -pthread -fsanitize=address -o sorter sorter.o sortFile.o
sorter.o: sorter.c sortFile.h
	gcc -c sorter.c
sortFile.o: sortFile.c sortFile.h
	gcc -c sortFile.c
clean:
	rm -f sorter sorter.o sortFile.o
