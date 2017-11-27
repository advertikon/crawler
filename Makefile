TARGET = crawler
PREFIX = /usr/local/bin
PWD = `pwd`

.PHONY: all clean install uninstall

all: $(TARGET)
	
clean:
			rm -rf $(TARGET) *.o
main.o: main.c
			gcc -c -o main.o $(PWD)/main.c -I /usr/include/libxml2 `pkg-config --cflags gtk+-3.0`
path.o: path.c
			gcc -c -o path.o path.c `pkg-config --cflags gtk+-3.0`
# structure.o: structure.c
# 			gcc -c -o structure.o structure.c
# args.o: args.c
# 			gcc -c -o args.o args.c
# error.o: error.c
# 			gcc -c -o error.o error.c

$(TARGET): path.o main.o #structure.o args.o main.o error.o
			gcc -o $(TARGET) path.o main.o -lm -I /usr/include/libxml2 -lxml2 `pkg-config --libs gtk+-3.0`
install:
			install $(TARGET) $(PREFIX)
uninstall:
			rm -rf $(PREFIX)/$(TARGET)