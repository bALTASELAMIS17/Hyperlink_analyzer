all: hyperlink_info test_parser

tests: all test_parser

# Build main program. -lcurl is required to properly link curl libraries.
hyperlink_info: hyperlink_info.c curling.c curling.h
	gcc -Wall -o $@ $^ -lcurl

test_parser: test_parser.c parser.c curling.c
	gcc -Wall -g -o $@ $^ -lcurl

clean:
	rm -f hyperlink_info test_parser