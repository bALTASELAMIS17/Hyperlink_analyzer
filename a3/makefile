GCC = gcc -Wall -g

all: html_files hyperlink_info
 
hyperlink_info: hyperlink_info.o dfs.o parser.o curling.o
	$(GCC) -o $@ $^ -lcurl
 
hyperlink_info.o: hyperlink_info.c curling.h dfs.h
	$(GCC) -c hyperlink_info.c -o $@
 
dfs.o: dfs.c parser.h
	$(GCC) -c dfs.c -o $@
 
parser.o: parser.c parser.h curling.h
	$(GCC) -c parser.c -o $@
 
curling.o: curling.c curling.h
	$(GCC) -c curling.c -o $@
 
html_files:
	mkdir -p html_files
 
clean:
	rm -f hyperlink_info hyperlink_info.o dfs.o parser.o curling.o
	rm -f status.txt processes.txt discovered_urls.txt
	rm -rf html_files
 
.PHONY: all clean html_files