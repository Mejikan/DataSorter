all: sorter_server.o xmlparse.o sortFile.o
	gcc -Wall -Werror -pthread -fsanitize=address -o sorter_server sorter_server.o xmlparse.o sortFile.o
sorter_server.o: sorter_server.c xmlparse.h sortFile.h
	gcc -c sorter_server.c
xmlparse.o: xmlparse.c xmlparse.h
	gcc -c xmlparse.c
sortFile.o: sortFile.c sortFile.h
	gcc -c sortFile.c
clean:
	rm -f sorter_server sorter_server.o xmlparse.o sortFile.o