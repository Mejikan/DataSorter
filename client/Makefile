all: sorter.o xmlparse.o sortFile.o
	gcc -Wall -Werror -pthread -fsanitize=address -o sorter sorter.o xmlparse.o sortFile.o
sorter.o: sorter.c xmlparse.h sortFile.h
	gcc -c sorter.c
xmlparse.o: xmlparse.c xmlparse.h
	gcc -c xmlparse.c
sortFile.o: sortFile.c sortFile.h
	gcc -c sortFile.c
clean:
	rm -f sorter sorter.o xmlparse.o sortFile.o
