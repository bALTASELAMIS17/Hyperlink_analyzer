#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <curl/curl.h>
#include "parser.h"
#include "curling.h"
#include <dirent.h>

#define BASE_URL "https://wikipedia.org" 

// returns 0 if label is unique, returns 1 if not
int label_exists(char **labels, int count, const char *label) {
	for (int i = 0; i < count; i++) {
		if (strcasecmp(labels[i], label) == 0) {
			return 1;
		}
	}
	return 0;
}

// Stores number of unique hyperlink labels and their links and returns how many were found
int extract_hyperlinks(const char *filepath, char ***labels_out, char ***links_out) {

	// opens the file 
	FILE *fp = fopen(filepath, "r"); 
	if (fp == NULL) {
		perror("fopen error");
		exit(EXIT_FAILURE);
	}


	// holds a line of the file
	char file_line[2000];
	int capacity = 20;
	int count = 0;

	char **labels = malloc(capacity * sizeof(char *));
	char **links  = malloc(capacity * sizeof(char *)); 
	
	if ((labels == NULL) || (links == NULL)) {
        perror("ERROR: malloc failed");
        exit(EXIT_FAILURE);
    }

    // goes through each line of the file
    while (fgets(file_line, sizeof(file_line), fp) != NULL) {
		char *line_pointer = file_line;

		// finds the start of a hyperlink label
		while ((line_pointer = strstr(line_pointer, "<a href=\"/wiki")) != NULL) {
			
			// extracting the link
			char *link_start = strstr(line_pointer, "href=\"");
			if (!link_start) break;
			link_start += 6;

			char *link_end = strchr(link_start, '"');
			if (!link_end) break;
			int link_len = link_end - link_start;

			// memory allocate link space 
			char *link = malloc(strlen(BASE_URL) + link_len + 1);
			if (link == NULL) {
		        perror("ERROR: malloc failed");
		        exit(EXIT_FAILURE);
		    }

		    // building the link
		    snprintf(link, strlen(BASE_URL) + link_len + 1, "%s%.*s", BASE_URL, link_len, link_start);

		    // extracting the label
			char *start = strchr(line_pointer, '>');
			if (!start) {
				free(link); 
				break;
			}
			start++; // moves past '>'

			char *end = strstr(start, "</a>");
			if (!end) {
				free(link);
				break;
			}

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
		    	free(link);
		    	line_pointer = end + 4;
		    	continue;
		    }

		    // checking if label is unique
		    if (label_exists(labels, count, label) == 0) {

		    	// if there are more labels than existing capacity
		    	if (count == capacity) {
		    		capacity *= 2;
		    		char **temp_labels = realloc(labels, capacity * sizeof(char *));
		    		if (temp_labels == NULL) {
					    free_all(labels, links, count);
					    exit(EXIT_FAILURE);
					}

		    		char **temp_links  = realloc(links,  capacity * sizeof(char *));
		    		if (temp_links  == NULL) {
					    free_all(temp_labels, links, count);
					    exit(EXIT_FAILURE);
					}

				    labels = temp_labels;
				    links = temp_links;
		    	}

		    	labels[count] = label;
		    	links[count] = link;
		    	count++;
		    }

		    else {
		    	free(label);
		    	free(link);
		    }

		    line_pointer = end + 4;
		}
	}
	fclose(fp);
	
	*labels_out = labels;
	*links_out = links;

	return count;
}

// Frees memory for hyperlink labels and links 
void free_all(char **labels, char **links, int count) {
	for (int i = 0; i < count; i++) {
		free(labels[i]);
		free(links[i]);
	}
	free(labels);
	free(links); 
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

// Takes in a wikipedia link and returns hyperlink count and stores links and labels
int hyperlink_analyzer (const char *url, char ***labels_out, char ***links_out) {
	const char *temp_file = "main_url_html.txt";

	char *result = get_html_from_url((char *)url, (char *)temp_file);

	if (result == NULL) {
		fprintf(stderr, "Failed to fetch HTML from url\n");
	}

    return extract_hyperlinks(temp_file, labels_out, links_out);
}

void cleanup_html_files() {
	DIR *dir;
	struct dirent *entry;; // represents one file at a time

	dir = opendir("html_files"); // opens the html_files folder

	if (dir == NULL) {
		perror("opendir failed"); 
		return;
	}

	char filepath[500]; // buffer storing filepath

	while ((entry = readdir(dir)) != NULL) { // goes through every file in directory
		if (strncmp(entry->d_name, "main_url_html", 13) == 0) { // if filename starts with desired tag
			snprintf(filepath, sizeof(filepath), "html_files/%s", entry->d_name); // builds full path

			if (remove(filepath) == 0) { // checks for successful deletion
				printf("Deleted %s\n", filepath);
			}
		}
	}

	closedir(dir);	
}