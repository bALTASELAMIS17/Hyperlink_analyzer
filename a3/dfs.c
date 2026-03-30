#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
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

void free_hyperlinks(char **labels, char **links, int num_links){
    for (int i = 0; i < num_links; i++) {
        free(labels[i]);
        free(links[i]);
    }
    free(labels);
    free(links);
}

int depth_first_search(char *current_url, char *end_url, int depth){
    // Create arrays for urls and labels, call to get the neighboring hyperlinks and their information
    char **urls = NULL;
    char **labels = NULL;
    int num_links = hyperlink_analyzer(current_url, &labels, &urls);
    printf("%d neighbors found for %s\n", num_links, current_url);
    if (num_links <= 0) {
        free_hyperlinks(labels, urls, num_links);
        return -1;
    }
    // Creates pipe fds for children -> parent communication
    int pipes[MAX_LINKS][2];
    int children_pids[MAX_LINKS];
    int num_of_children = 0;
    // Loop through every neighbour of the current article
    if (depth >= 1) { // TESTING
        printf("Depth of 2 reached, quit\n");
        free_hyperlinks(labels, urls, num_links);
        return -1;
    }
    for (int i = 0; i < num_links; i++){
        printf("%s\n", urls[i]);
        // TODO: Check if we have exceeded the max number of processes. If so, quit. If not, increment
        // TODO: Check if the url is in the file (meaning it already has been seen). If so, quit. If not, continue
        // Check if the url is the right one. If so, directly return the current depth
        if (strcmp(end_url, urls[i]) == 0){
            printf("======================== MATCH IS FOUND\n");
            for (int j = 0; j < num_of_children; j++) {
                close(pipes[j][0]);
                waitpid(children_pids[j], NULL, 0);
            }
            free_hyperlinks(labels, urls, num_links);
            return depth + 1;
        }
        // If the url is not the right one, make a new pipe, make a new child, ask the child to run another extract_hyperlinks
        else {
            // Create pipe
            if (pipe(pipes[num_of_children]) == -1) {
                perror("pipe");
                continue;
            }
            // Create child
            int r = fork();
            if (r < 0){
                perror("fork");
                close(pipes[num_of_children][0]);
                close(pipes[num_of_children][1]);
                continue;
            }
            // If child, close read end, run extract_hyperlinks, write depth, close pipe
            if (r == 0){
                close(pipes[num_of_children][0]);
                int result = depth_first_search(urls[i], end_url, depth + 1);
                if (write(pipes[num_of_children][1], &result, sizeof(int)) != sizeof(int)){
                    perror("write");
                }
                // printf("Child spawned for label %s, depth of %d, result of %d\n", labels[i], depth + 1, result);
                close(pipes[num_of_children][1]);
                free_hyperlinks(labels, urls, num_links);
                exit(0);
            }
            // If parent, close write end, record pid of child
            else{
                close(pipes[num_of_children][1]);
                children_pids[num_of_children] = r;
                num_of_children++;
            }
        }
    }
    // Wait for all the children to finish, loop through the array, find if any has non -1 result
    int result = -1;
    for (int i = 0; i < num_of_children; i++){
        int child_result = -1;
        // Check if read is done successfully
        if (read(pipes[i][0], &child_result, sizeof(int)) == sizeof(int)){
            // Modify results if: child result is non -1 and : 1. result is -1, 2. result is greater than child result
            if ((child_result != -1) && (result == -1 || result > child_result)){
                printf("Success found\n");
                result = child_result;
            }
        }
        close(pipes[i][0]);
        waitpid(children_pids[i], NULL, 0);
    }
    free_hyperlinks(labels, urls, num_links);
    return result;
}

int main(void){
    int result = depth_first_search("https://en.wikipedia.org/wiki/Webber_Academy", "https://en.wikipedia.org/wiki/Calgary", 0);
    printf("Final result: %d\n", result);
    return 0;
}
