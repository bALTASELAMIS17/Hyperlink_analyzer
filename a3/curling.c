#include <stdio.h>
#include <curl/curl.h>

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/142.0.0.0 Safari/537.36"


size_t write_to_file(void *ptr, size_t size, size_t nmemb, void *userdata) {
    FILE *fp = (FILE *)userdata;
    return fwrite(ptr, size, nmemb, fp);
}


//Returns file name
char* get_html_from_url(char *URL, char *file_name){
    //download location of html contents of website
    FILE *fp = fopen(file_name, "w");
    if (!fp) {
        perror("fopen");
        return NULL;
    }

    //I just used whatever libcurl documentation says for easy curl
    CURL *curl;
    CURLcode result;
    char errbuf[CURL_ERROR_SIZE];

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl){
        errbuf[0] = '\0';

        curl_easy_setopt(curl, CURLOPT_URL, URL);
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
            } else {
                fprintf(stderr, "libcurl error: %s\n", curl_easy_strerror(result));
            }
        }

        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "curl_easy_init failed.\n");
    }

    // Appease the valgrind gods.
    fclose(fp);
    curl_global_cleanup();

    return file_name;
}