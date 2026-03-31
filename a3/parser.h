#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

// Checks if label is unique
int label_exists(char **labels, int count, const char *label);

// Stores unique hyperlink labels and their links and returns how many were found
int extract_hyperlinks(const char *filepath, char ***labels_out, char ***links_out);

// Strips html tags to find label names
void strip_html_tags(char *text);

// Frees memory for hyperlink labels and links
void free_all(char **labels, char **links, int count);

// Takes in a wikipedia link and returns hyperlink count and stores links and labels
int hyperlink_analyzer (const char *url, char ***labels_out, char ***links_out);

// Deletes all files in the the html_files folder
void cleanup_html_files(); 

#endif
