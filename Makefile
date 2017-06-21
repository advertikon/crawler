TARGET = crawler
PREFIX = /usr/local/bin

.PHONY: all clean install uninstall

all: $(TARGET)
	
clean:
			rm -rf $(TARGET) *.o
main.o: main.c
			gcc -c -o main.o main.c
hello.o: path.c
			gcc -c -o path.o path.c
structure.o:
			gcc -c -o structure.o structure.c
args.o:
			gcc -c -o args.o args.c
$(TARGET): path.o structure.o args.o main.o
			gcc -o $(TARGET) path.o args.o structure.o main.o
install:
			install $(TARGET) $(PREFIX)
uninstall:
			rm -rf $(PREFIX)/$(TARGET)