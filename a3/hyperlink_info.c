#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curling.h"
#include "dfs.h"

#define DEFAULT_START_URL "https://en.wikipedia.org/wiki/Mimico_Creek"
#define DEFAULT_END_URL "https://wikipedia.org/wiki/Calgary"
#define DEFAULT_DEPTH 2
#define MAX_PROCESSES 200

int main(int argc, char **argv){
    //Debugging cli arguments
    printf("Program has recieved %d arguments:\n", argc);
    for (int i = 0; i < argc; i++){
        printf(" - Argument %d: %s\n", i, argv[i]);
    }
    printf("=======================================================================\n\n");

    char *start_url = DEFAULT_START_URL;
    char *end_url   = DEFAULT_END_URL;
    int depth       = DEFAULT_DEPTH;

    
    if (strncmp(argv[2], "https://wikipedia.org/wiki/", 26) != 0) {
        fprintf(stderr, "Error: END_URL must be of the format \"https://wikipedia.org/wiki/[article]\"\n");
        return 1;
    } else if (argc == 4) {
        start_url = argv[1];
        end_url   = argv[2];
        depth     = atoi(argv[3]);
    } else {
        printf("Invalid number of arguments.\n");
        printf("Using default START_URL: %s\n", DEFAULT_START_URL);
        printf("Using default END_URL: %s\n", DEFAULT_END_URL);
        printf("Using default depth: %d\n", DEFAULT_DEPTH);
        printf("Usage: %s \"START_URL\" \"https://wikipedia.com/wiki/[article]\" depth\n", argv[0]);
    }

    printf("Start URL: %s\n", start_url);
    printf("End URL: %s\n", end_url);
    printf("Depth: %d\n", depth);
    printf("\n");

    start_search(start_url, end_url, depth, MAX_PROCESSES);

    return 0;
}
