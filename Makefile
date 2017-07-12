TARGET = crawler
PREFIX = /usr/local/bin

.PHONY: all clean install uninstall

all: $(TARGET)
	
clean:
			rm -rf $(TARGET) *.o
main.o: main.c
			gcc -c -o main.o main.c -I /usr/include/libxml2
path.o: path.c
			gcc -c -o path.o path.c
structure.o: structure.c
			gcc -c -o structure.o structure.c
args.o: args.c
			gcc -c -o args.o args.c
error.o: error.c
			gcc -c -o error.o error.c

$(TARGET): path.o structure.o args.o main.o error.o
			gcc -o $(TARGET) path.o args.o structure.o main.o error.o -llibyaml -lm -I /usr/include/libxml2 -lxml2
install:
			install $(TARGET) $(PREFIX)
uninstall:
			rm -rf $(PREFIX)/$(TARGET)