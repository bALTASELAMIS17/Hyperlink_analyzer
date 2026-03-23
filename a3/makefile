all: hyperlink_info

#Build main program. -lcurl is required to properly link curl libraries.
hyperlink_info: hyperlink_info.c
	gcc -Wall -o $@ $^ -lcurl