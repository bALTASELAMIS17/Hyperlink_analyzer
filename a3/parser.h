#ifndef PARSER_H 
#define PARSER_H

#include <stdio.h>

// Counts the frequency of a word in a file, 0 for raw count, 1 for user count
int count_frequency(FILE *fp, const char *target, int mode);

// Checks if label is unique
int label_exists(char **labels, int count, const char *label); 

// Stores number of unique hyperlink labels and returns how many were found
int extract_hyperlink(FILE *fp, char ***labels_out);

// Returns 1 if target is found within file, 0 if not
int find_hyperlink(FILE *fp, const char *target);

// Strips html tags to find label names
void strip_html_tags(char *text);

// Frees memory for hyperlink labels
void free_labels(char **labels, int count);

#endif