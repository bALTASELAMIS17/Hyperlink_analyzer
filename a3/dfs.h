#ifndef DFS_H
#define DFS_H

#include <stdio.h>

// Deletes all files in the the html_files folder
void cleanup_html_files();

int start_search(char *start_url, char *end_url, int max_depth, int max_processes);

#endif
