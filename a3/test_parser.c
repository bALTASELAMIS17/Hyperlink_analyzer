#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

int main() {
	char **labels;
	char **links;

	int hyperlink_count = hyperlink_analyzer(
	    "https://wikipedia.org/wiki/Ashtone_Morgan",
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