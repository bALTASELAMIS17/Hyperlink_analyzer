#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/file.h>
#include "parser.h"

#define MAX_LINKS 1000
#define MAX_URL_LEN 100

void initialize_processes_file(){
    int fd = open("processes.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    int zero = 0;
    write(fd, &zero, sizeof(int));
    close(fd);
}

bool process_limit_check(int max_processes){
    char *filepath = "processes.txt";
    int fd = open(filepath, O_RDWR);
    if (fd == -1){
        perror("open");
        exit(1);
    }
    int processes = -1;
    if (read(fd, &processes, sizeof(int)) != sizeof(int)){
        perror("read");
        close(fd);
        exit(1);
    }
    printf("Currently running processes: %d\n", processes);
    if (processes >= max_processes) {
        close(fd);
        return false;
    }
    else{
        processes++;
        if (lseek(fd, 0, SEEK_SET) == -1) {
            perror("lseek");
            close(fd);
            exit(1);
        }

        if (write(fd, &processes, sizeof(int)) != sizeof(int)) {
            perror("write");
            close(fd);
            exit(1);
        }

        close(fd);
        return true;
    }
}

void process_end(){
    char *filepath = "processes.txt";
    int fd = open(filepath, O_RDWR);
    if (fd == -1){
        perror("open");
        exit(1);
    }
    int processes = -1;
    if (read(fd, &processes, sizeof(int)) != sizeof(int)){
        perror("read");
        close(fd);
        exit(1);
    }
    if (processes > 0) {
        processes--;
        if (lseek(fd, 0, SEEK_SET) == -1){
            perror("lseek");
            close(fd);
            exit(1);
        }
        if (write(fd, &processes, sizeof(int)) != sizeof(int)){
            perror("write");
            close(fd);
            exit(1);
        }
    }
    close(fd);
}

#define URLS_FILE "discovered_urls.txt"
#define MAX_LINE 1024

void initialize_urls_file(void) {
    int fd = open(URLS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    close(fd);
}


bool check_and_add_discovered_url(const char *url) {
    int fd = open(URLS_FILE, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    //Lock fp to prevent racing of children (blocks other processes).
    if (flock(fd, LOCK_EX) == -1) {
        perror("flock");
        close(fd);
        exit(1);
    }

    FILE *fp = fdopen(fd, "r+");
    if (fp == NULL) {
        perror("fdopen");
        close(fd);
        exit(1);
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, url) == 0) {
            flock(fd, LOCK_UN);
            fclose(fp);
            return true;
        }
    }

    fprintf(fp, "%s\n", url);
    fflush(fp);

    flock(fd, LOCK_UN);
    fclose(fp);
    return false;
}

void free_hyperlinks(char **labels, char **links, int num_links){
    for (int i = 0; i < num_links; i++) {
        free(labels[i]);
        free(links[i]);
    }
    free(labels);
    free(links);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_QUEUE_SIZE 100000

// Each node in the queue has url and depth
typedef struct {
    char *url;
    int depth;
} QueueNode;

int breadth_first_search(char *current_url, char *end_url, int depth, int max_depth) {
    QueueNode queue[MAX_QUEUE_SIZE];
    int start_index = 0;
    int end_index = 0;

    // mark the starting URL as discovered
    check_and_add_discovered_url(current_url);

    queue[end_index].url = strdup(current_url);
    queue[end_index].depth = depth;
    end_index++;

    // While the queue is non empty
    while (start_index < end_index) {
        QueueNode current = queue[start_index];
        start_index++;

        printf("Finding neighbors of %s at depth %d\n", current.url, current.depth);

        // Depth limit
        if (current.depth >= max_depth) {
            free(current.url);
            continue;
        }

        // rate limit
        usleep(200000 + (rand() % 200000));

        char **urls = NULL;
        char **labels = NULL;
        int num_links = hyperlink_analyzer(current.url, &labels, &urls);
        printf("%d neighbors found for %s\n", num_links, current.url);
        if (num_links <= 0) {
            free_hyperlinks(labels, urls, num_links);
            free(current.url);
            continue;
        }
        for (int i = 0; i < num_links; i++) {
            // check match
            if (strcmp(end_url, urls[i]) == 0) {
                printf("======================== MATCH IS FOUND\n");
                free_hyperlinks(labels, urls, num_links);
                free(current.url);
                // if match is found, clear the queue
                while (start_index < end_index) {
                    free(queue[start_index].url);
                    start_index++;
                }
                return current.depth + 1;
            }

            // skip already discovered
            if (check_and_add_discovered_url(urls[i])) {
                printf("Skipping already discovered URL: %s\n", urls[i]);
                continue;
            }

            if (end_index >= MAX_QUEUE_SIZE) {
                fprintf(stderr, "Queue filled\n");
                free_hyperlinks(labels, urls, num_links);
                free(current.url);

                while (start_index < end_index) {
                    free(queue[start_index].url);
                    start_index++;
                }

                return -1;
            }

            queue[end_index].url = strdup(urls[i]);
            queue[end_index].depth = current.depth + 1;
            end_index++;
        }

        free_hyperlinks(labels, urls, num_links);
        free(current.url);
    }

    return -1;
}


int main(void) {
    initialize_urls_file();
    check_and_add_discovered_url("https://en.wikipedia.org/wiki/Webber_Academy");

    int result = breadth_first_search(
        "https://en.wikipedia.org/wiki/Webber_Academy",
        "https://wikipedia.org/wiki/Western_Canada",
        0,
        3
    );

    printf("Final result: %d\n", result);
    return 0;
}
