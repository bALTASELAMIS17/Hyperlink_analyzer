#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

int main() {
	FILE *fp = fopen("main_url_html.txt", "r"); 
	if (fp == NULL) {
		perror("fopen error");
		exit(EXIT_FAILURE);
	}

	char **labels;
	int hyperlink_count = extract_hyperlink(fp, &labels); 

	printf("Found %d unique labels in the file\n", hyperlink_count);

	for (int i = 0; i < hyperlink_count; i++) {
		printf("%s\n", labels[i]);
	}

	
	if (find_hyperlink(fp, "Toronto FC") == 1) {
		printf("Toronto FC raw count: %d\n", count_frequency(fp, "Toronto FC", 0));
		printf("Toronto FC user count: %d\n", count_frequency(fp, "Toronto FC", 1));
	}
	else {
		printf("Toronto FC was not found in the file\n");
	}

	printf("Real Salt Lake error count: %d\n", count_frequency(fp, "Real Salt Lake", 3));

	if (find_hyperlink(fp, "Justin Morrow") == 1) {
		printf("Justin Morrow user count: %d\n", count_frequency(fp, "Justin Morrow", 1));
	}
	else {
		printf("Justin Morrow was not found in the file\n");
	}

	free_labels(labels, hyperlink_count); 

	fclose(fp);

	return 0;
}