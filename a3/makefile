all: hyperlink_info parser

tests: all test_parser

#Build main program. -lcurl is required to properly link curl libraries.
hyperlink_info: hyperlink_info.c curling.c curling.h
	gcc -Wall -o $@ $^ -lcurl

test_parser: test_parser.c parser.h parser.c
	gcc -Wall -o $@ $^