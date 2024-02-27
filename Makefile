.phony all:
all: mts

mts: mts.c
ifdef d
	gcc -std=c99 -g mts.c -pthread -lreadline -lhistory -ltermcap -o mts
else
	gcc mts.c -pthread -lreadline -lhistory -ltermcap -o mts 
endif

.PHONY clean:
clean:
	-rm -rf *.o *.exe
