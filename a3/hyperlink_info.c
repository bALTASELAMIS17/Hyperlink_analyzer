#include <stdio.h>
#include <curl/curl.h>

int main(int argv, char **argc){
    // Debugging cli arguments
    printf("[DEBUG]: I have received %d arguments:\n", argv - 1);
    for (int i = 0; i < argv; i++){
        printf(" - Argument %d: %s\n", i, argc[i]);
    }
    // Debugging end.

    CURL *curl;
    curl = curl_easy_init(); 


}