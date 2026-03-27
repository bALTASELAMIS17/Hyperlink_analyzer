#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "parser.h"

#define MAX_LABELS 1000
#define MAX_LINE 4000

// mode 0 for raw count, mode 1 for user count
int count_frequency(FILE *fp, const char *target, int mode) {

	// target error check 
	if (target == NULL || strlen(target) == 0) {
		return 0;
	}

	// mode error check
	if (mode != 0 && mode != 1) {
		fprintf(stderr, "mode must be 0 (raw count) or 1 (user count)\n");
		return -1;
	}

	// resets pointer back to beginning of file
	rewind(fp); 

	// holds a line of the file
	char file_line[1000];
	int count = 0;
	int inside_text = 0; 

	// goes through each line of the file
	while (fgets(file_line, sizeof(file_line), fp) != NULL) {
		
		// want to read raw count
		if (mode == 0) {
			char *line_pointer = file_line;

			// checks in each line if there's a match with the desired target
			while ((line_pointer = strcasestr(line_pointer, target)) != NULL) {
				count++; 
				line_pointer += strlen(target);
			}

		}
	    
	    // mode 1 for user count
		if (mode == 1) {
			// detecting paragraph start
			if (strstr(file_line, "<p") != NULL) {
	        	inside_text = 1;
		    }

		    if (!(inside_text)) {
			    continue;
			}

			strip_html_tags(file_line);

			char *line_pointer = file_line;

			// checks in each line if there's a match with the desired target
			while ((line_pointer = strcasestr(line_pointer, target)) != NULL) {
				count++; 
				line_pointer += strlen(target);
			}
			
			// detecting paragraph end
			if (strstr(file_line, "</p>") != NULL) {
	        	inside_text = 0;
	    	}
	    }
	}

	return count;
}

// returns 0 if label is unique, returns 1 if not
int label_exists(char **labels, int count, const char *label) {
	for (int i = 0; i < count; i++) {
		if (strcasecmp(labels[i], label) == 0) {
			return 1;
		}
	}
	return 0; 
}

int extract_hyperlink(FILE *fp, char ***labels_out) {

	// holds a line of the file
	char file_line[2000];
	int capacity = 20;
	int count = 0;

	char **labels = malloc(capacity * sizeof(char *));
	
	if (labels == NULL) {
        perror("ERROR: malloc failed");
        exit(EXIT_FAILURE);
    }

    // goes through each line of the file
    while (fgets(file_line, sizeof(file_line), fp) != NULL) {
		char *line_pointer = file_line;

		// finds the start of a hyperlink label
		while ((line_pointer = strstr(line_pointer, "<a href=\"/wiki")) != NULL) {
			
			char *start = strchr(line_pointer, '>');
			if (!start) break;
			start++; // moves past '>'

			char *end = strstr(start, "</a>");
			if (!end) break;

			// bad labels
			int len = end - start;
			if (len <= 0) {
				line_pointer = end + 4; // moves past '</a>'
				continue; 
			}

			// memory allocate label
			char *label = malloc(len+1); 

			if (label == NULL) {
		        perror("ERROR: malloc failed");
		        exit(EXIT_FAILURE);
		    }

		    strncpy(label, start, len);
		    label[len] = '\0'; // null terminate label

		    strip_html_tags(label); 

		    // if label is empty or too short, skip and free memory
		    if (strlen(label) <= 1) {
		    	free(label);
		    	line_pointer = end + 4;
		    	continue;
		    }

		    // checking if label is unique
		    if (label_exists(labels, count, label) == 0) {

		    	// if there are more labels than existing capacity
		    	if (count == capacity) {
		    		capacity *= 2;
		    		char **temp = realloc(labels, capacity * sizeof(char *));
		    		// safely deal with realloc failure
		    		if (temp == NULL) {
				        perror("ERROR: realloc failed");
				        free_labels(labels, count);
				        exit(EXIT_FAILURE);
				    }

				    labels = temp;
		    	}

		    	labels[count++] = label;
		    }

		    else {
		    	free(label);
		    }

		    line_pointer = end + 4;
		}
	}

	*labels_out = labels;
	return count;
}

void free_labels(char **labels, int count) {
	for (int i = 0; i < count; i++) {
		free(labels[i]);
	}
	free(labels);
}

// Strips html tags to find label names
void strip_html_tags(char *text) {
	char *read_pointer = text;
	char *write_pointer = text;
	int inside_tag = 0;

	while (*read_pointer) {

		if (*read_pointer == '<') {
			inside_tag = 1;
		}

		else if (*read_pointer == '>') {
			inside_tag = 0;
			read_pointer++; 
			continue;
		}

		else if (inside_tag == 0) {
			*write_pointer++ = *read_pointer; 
		}

		read_pointer++;
	}

	*write_pointer = '\0';
}