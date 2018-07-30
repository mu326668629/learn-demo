
CFLAGS=-DDEBUG -g
APP=timer

$(APP) : etimer.o rbtree.o
	gcc -o $(APP) $^

clean:
	rm -f $(APP) *.o
