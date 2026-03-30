#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

int main(int argc, char **argv) {
	char **labels;
	char **links;

	if (argc != 2) {
        printf("Invalid Input");
        return 1;
    }

	int hyperlink_count = hyperlink_analyzer(
	    argv[1],
	    &labels,
	    &links
	);

	printf("Found %d unique labels in the file\n\n", hyperlink_count);

	for (int i = 0; i < hyperlink_count; i++) {
		printf("Label: %s\n", labels[i]);
		printf("Link: %s\n", links[i]);
	}

	free_all(labels, links, hyperlink_count); 

	return 0;
}