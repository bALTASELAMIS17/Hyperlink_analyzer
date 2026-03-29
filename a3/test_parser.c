#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

int main() {
	char **labels;
	char **links;
	int hyperlink_count = extract_hyperlinks("main_url_html.txt", &labels, &links); 

	printf("Found %d unique labels in the file\n\n", hyperlink_count);

	for (int i = 0; i < hyperlink_count; i++) {
		printf("%s\n", labels[i]);
	}

	printf("\n");

	for (int i = 0; i < hyperlink_count; i++) {
		printf("%s\n", links[i]);
	}

	free_all(labels, links, hyperlink_count); 

	return 0;
}