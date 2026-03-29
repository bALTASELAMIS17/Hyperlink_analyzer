#include <stdio.h>
#include "curling.h"

#define DEFAULT_MAIN_URL "https://wikipedia.org"

int main(int argc, char **argv){
    // Debugging cli arguments
    printf("[DEBUG]: I have received %d arguments:\n", argc);
    for (int i = 0; i < argc; i++){
        printf(" - Argument %d: %s\n", i, argv[i]);
    }
    // Debugging end.

    char *MAIN_URL = DEFAULT_MAIN_URL;

    if (argc == 1) {
        printf("No arguments provided, using default URL: %s\n", DEFAULT_MAIN_URL);
    } else if (argc > 2) {
        printf("Usage: %s \"Website URL\"\n", argv[0]);
        return 1;
    } else {
        MAIN_URL = argv[1];
    }

    get_html_from_url(MAIN_URL, "main_url_html.txt");

    return 0;
}