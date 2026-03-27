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

	printf("Toronto FC raw count: %d\n", count_frequency(fp, "Toronto FC", 0));
	printf("Toronto FC user count: %d\n", count_frequency(fp, "Toronto FC", 1));

	printf("Philadelphia Union raw count: %d\n", count_frequency(fp, "Philadelphia Union", 0));
	printf("Philadelphia Union user count: %d\n", count_frequency(fp, "Philadelphia Union", 1));

	printf("Real Salt Lake error count: %d\n", count_frequency(fp, "Real Salt Lake", 3));

	printf("Justin Morrow user count: %d\n", count_frequency(fp, "Justin Morrow", 1));

	free_labels(labels, hyperlink_count); 

	fclose(fp);

	return 0;
}