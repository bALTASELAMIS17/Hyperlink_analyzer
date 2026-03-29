#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <curl/curl.h>
#include "parser.h"

#define USER_AGENT "Mozilla/5.0"
#define TEMP_FILE "main_url_html.txt"
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

// Writes to the file 
size_t write_to_file(void *ptr, size_t size, size_t nmemb, void *userdata) {
    FILE *fp = (FILE *)userdata;
    return fwrite(ptr, size, nmemb, fp);
}

// Takes in a wikipedia link and returns hyperlink count and stores links and labels
int hyperlink_analyzer (const char *url, char ***labels_out, char ***links_out) {
	FILE *fp = fopen("main_url_html.txt", "w");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    CURL *curl;
    CURLcode result;
    char errbuf[CURL_ERROR_SIZE];

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl){
        errbuf[0] = '\0';

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        //Follow redirects.
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        //Set User agent to prevent looking like a bot.
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);

        //Set ouput of curl. Right now saves to file, may need to allocate heap and place in memory.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        result = curl_easy_perform(curl);

        if (result != CURLE_OK) {
            if (errbuf[0] != '\0') {
                fprintf(stderr, "libcurl error: %s\n", errbuf);
                return 0;
            } else {
                fprintf(stderr, "libcurl error: %s\n", curl_easy_strerror(result));
                return 0;
            }
        }

        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "curl_easy_init failed.\n");
    }

    // Appease the valgrind gods.
    fclose(fp);
    curl_global_cleanup();

    return extract_hyperlinks(TEMP_FILE, labels_out, links_out);
}