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

int breadth_first_search(char *current_url, char *end_url, int depth) {
    char **urls = NULL;
    char **labels = NULL;

    //rate limiter
    usleep(200000 + (rand() % 200000));
    int num_links = hyperlink_analyzer(current_url, &labels, &urls);
    printf("%d neighbors found for %s\n", num_links, current_url);

    if (num_links <= 0) {
        free_hyperlinks(labels, urls, num_links);
        return -1;
    }

    // Maximum depth limit
    if (depth >= 2) {
        printf("Depth of 3 reached, quit\n");
        free_hyperlinks(labels, urls, num_links);
        return -1;
    }

    // Check all neighbors
    for (int i = 0; i < num_links; i++) {
        printf("%s\n", urls[i]);

        // See if there is a match
        if (strcmp(end_url, urls[i]) == 0) {
            printf("======================== MATCH IS FOUND\n");
            free_hyperlinks(labels, urls, num_links);
            return depth + 1;
        }
    }

    // Creates pipes
    int pipes[num_links][2];
    int children_pids[num_links];
    int num_children = 0;

    // Loop through all neighbors
    for (int i = 0; i < num_links; i++) {

        if (check_and_add_discovered_url(urls[i])) {
            printf("Skipping already discovered URL: %s\n", urls[i]);
            continue;
        }

        // Make pipe
        if (pipe(pipes[num_children]) == -1) {
            perror("pipe");
            free_hyperlinks(labels, urls, num_links);
            return -1;
        }

        // Fork
        int r = fork();
        if (r < 0) {
            perror("fork");
            close(pipes[num_children][0]);
            close(pipes[num_children][1]);
            free_hyperlinks(labels, urls, num_links);
            return -1;
        }

        // Child: close read end, conduct search, write depth result to pipe
        if (r == 0) {
            close(pipes[num_children][0]);

            printf("Child searching %s at depth %d\n", urls[i], depth + 1);
            int result = breadth_first_search(urls[i], end_url, depth + 1);
            if (write(pipes[num_children][1], &result, sizeof(int)) != sizeof(int)) {
                perror("write");
                close(pipes[num_children][1]);
                exit(1);
            }

            close(pipes[num_children][1]);
            exit(0);
        }

        // Parent: store childs pid for use later, close write end
        children_pids[num_children] = r;
        close(pipes[num_children][1]);
        num_children++;
    }

    // Given the pid of the child processes, wait for all of them to finish.
    // If any of them returns non -1 and its currently -1, or if they have a better result, change best result
    int best_result = -1;
    for (int i = 0; i < num_children; i++) {
        int child_result = -1;
        if (read(pipes[i][0], &child_result, sizeof(int)) != sizeof(int)) {
            perror("read");
            child_result = -1;
        }
        close(pipes[i][0]);
        waitpid(children_pids[i], NULL, 0);

        if (child_result != -1) {
            if (best_result == -1 || child_result < best_result) {
                best_result = child_result;
            }
        }
    }

    free_hyperlinks(labels, urls, num_links);
    return best_result;
}
int main(void) {
    initialize_urls_file();
    check_and_add_discovered_url("https://en.wikipedia.org/wiki/Webber_Academy");

    int result = breadth_first_search(
        "https://en.wikipedia.org/wiki/Webber_Academy",
        "https://wikipedia.org/wiki/Western_Canada",
        0
    );

    printf("Final result: %d\n", result);
    return 0;
}
