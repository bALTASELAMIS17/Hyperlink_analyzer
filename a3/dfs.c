#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/file.h>
#include <stdint.h>
#include "parser.h"

#define MAX_LINKS 1000
#define MAX_URL_LEN 100
#define MAX_LINE 1024
#define PROCESSES_FILE "processes.txt"
#define MAX_PATH_LEN 4096
#define STATUS_FILE "status.txt"
#define STATUS_SEARCHING 0
#define STATUS_FOUND 1
#define STATUS_PROCESS_CAP 2
#define URLS_FILE "discovered_urls.txt"
#define RATELIMITER_TIME (1000000 * (depth * depth + 1) + (rand() % 200000))

// Initialize the status file (0 = searching, 1 = found, 2 = process cap reached)
void initialize_status_file(void) {
    int fd = open(STATUS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open status file");
        exit(1);
    }
    int status = STATUS_SEARCHING;
    if (write(fd, &status, sizeof(int)) != sizeof(int)) {
        perror("write status file");
        close(fd);
        exit(1);
    }
    close(fd);
}

// Read the status file and return the raw value
int read_status(void) {
    int fd = open(STATUS_FILE, O_RDONLY | O_CREAT, 0666);
    if (fd == -1) {
        perror("open status file");
        exit(1);
    }

    if (flock(fd, LOCK_SH) == -1) {
        perror("flock");
        close(fd);
        exit(1);
    }

    int status = 0;
    if (read(fd, &status, sizeof(int)) == -1) {
        perror("read status file");
        flock(fd, LOCK_UN);
        close(fd);
        exit(1);
    }

    flock(fd, LOCK_UN);
    close(fd);
    return status;
}

// Check if any termination condition has been met (found or process cap)
bool should_terminate(void) {
    return read_status() != STATUS_SEARCHING;
}

// Write a status code to the status file
void set_status(int new_status) {
    int fd = open(STATUS_FILE, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("open status file");
        exit(1);
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("flock");
        close(fd);
        exit(1);
    }
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        flock(fd, LOCK_UN);
        close(fd);
        exit(1);
    }
    if (write(fd, &new_status, sizeof(int)) != sizeof(int)) {
        perror("write status file");
        flock(fd, LOCK_UN);
        close(fd);
        exit(1);
    }
    flock(fd, LOCK_UN);
    close(fd);
}

// Initialize the file that stores whether the
void initialize_processes_file(){
    int fd = open(PROCESSES_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open processes file");
        exit(1);
    }
    int found = 0;
    if (write(fd, &found, sizeof(int)) != sizeof(int)){
        perror("write processes file");
        close(fd);
        exit(1);
    }
    close(fd);
}

// Build string with the path of the urls
void sprint_path(char *buffer, char *path[], int path_len) {
    int offset = 0;
    offset += snprintf(buffer + offset, MAX_PATH_LEN - offset, "\n============================================== PATH FOUND:\n");
    for (int i = 0; i < path_len; i++) {
        if (offset >= MAX_PATH_LEN) break;
        offset += snprintf(buffer + offset, MAX_PATH_LEN - offset, "[%d] %s\n", i, path[i]);
    }
    if (offset < MAX_PATH_LEN) {
        snprintf(buffer + offset, MAX_PATH_LEN - offset, "\n");
    }
}

//Read bytes from fd ensuring interrupted/chunked reads complete up to count
int read_full(int fd, void *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t r = read(fd, (char*)buf + total, count - total);
        if (r <= 0) break;
        total += r;
    }
    return total;
}

//Write bytes to fd ensuring interrupted/chunked writes complete up to count. 
//Incomplete chunking can be caused by scheduler remving proc from cpu.
int write_full(int fd, const void *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t w = write(fd, (const char*)buf + total, count - total);
        if (w <= 0) break;
        total += w;
    }
    return total;
}

// Check if the number of processes have exceeded the max
bool process_limit_check(int max_processes) {
    char *filepath = PROCESSES_FILE;
    int fd = open(filepath, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    if (flock(fd, LOCK_EX) == -1) {
        perror("flock");
        close(fd);
        exit(1);
    }
    int processes = 0;
    ssize_t bytes_read = read(fd, &processes, sizeof(int));

    if (bytes_read == -1) {
        perror("read processes limit increase");
        flock(fd, LOCK_UN);
        close(fd);
        exit(1);
    }
    if (bytes_read == 0) {
        processes = 0;
    } else if (bytes_read != sizeof(int)) {
        fprintf(stderr, "processes.txt has invalid size\n");
        flock(fd, LOCK_UN);
        close(fd);
        exit(1);
    }
    printf("Processes: (%d/%d) \n", processes, max_processes);

    if (processes >= max_processes) {
        flock(fd, LOCK_UN);
        close(fd);
        set_status(STATUS_PROCESS_CAP);
        return false;
    }
    processes++;
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        flock(fd, LOCK_UN);
        close(fd);
        exit(1);
    }
    if (write(fd, &processes, sizeof(int)) != sizeof(int)) {
        perror("write");
        flock(fd, LOCK_UN);
        close(fd);
        exit(1);
    }

    flock(fd, LOCK_UN);
    close(fd);
    return true;
}

// Decrement the processes file when a process ends
void process_end() {
    char *filepath = PROCESSES_FILE;
    int fd = open(filepath, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    if (flock(fd, LOCK_EX) == -1) {
        perror("flock");
        close(fd);
        exit(1);
    }
    int processes = 0;
    ssize_t bytes_read = read(fd, &processes, sizeof(int));

    if (bytes_read == -1) {
        perror("read processes limit decrease");
        flock(fd, LOCK_UN);
        close(fd);
        exit(1);
    }
    if (bytes_read == 0) {
        processes = 0;
    } else if (bytes_read != sizeof(int)) {
        fprintf(stderr, "processes.txt has invalid size\n");
        flock(fd, LOCK_UN);
        close(fd);
        exit(1);
    }

    if (processes > 0) {
        processes--;
        if (lseek(fd, 0, SEEK_SET) == -1) {
            perror("lseek");
            flock(fd, LOCK_UN);
            close(fd);
            exit(1);
        }
        if (write(fd, &processes, sizeof(int)) != sizeof(int)) {
            perror("write");
            flock(fd, LOCK_UN);
            close(fd);
            exit(1);
        }
    }
    flock(fd, LOCK_UN);
    close(fd);
}

// Initialize the file that stores the urls
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

// Let go of the hyperlinks and urls declared with malloc in parser.c
void free_hyperlinks(char **labels, char **links, int num_links){
    for (int i = 0; i < num_links; i++) {
        free(labels[i]);
        free(links[i]);
    }
    free(labels);
    free(links);
}

// Given current and target url, maximum depth, maximum number of processes, finds the path between the urls with DFS search
int depth_first_search(char *current_url, char *end_url, int depth, int max_depth, int max_processes, char *path[], int path_len, char *found_path_str_out) {
    char **urls = NULL;
    char **labels = NULL;
    int best_result = -1;
    char best_path_str[MAX_PATH_LEN] = "";

    if (should_terminate()) {
        return -1;
    }
    // Delay to prevent too frequent requests to Wikipedia
    usleep(RATELIMITER_TIME);
    int num_links = hyperlink_analyzer(current_url, &labels, &urls);
    printf("%d neighbors found for %s\n", num_links, current_url);
    if (num_links <= 0) {
        free_hyperlinks(labels, urls, num_links);
        return -1;
    }

    // Loop through all the neighboring hyperlinks to see if any of them match the target
    for (int i = 0; i < num_links; i++) {
        if (should_terminate()) {
            free_hyperlinks(labels, urls, num_links);
            return -1;
        }

        if (strcmp(end_url, urls[i]) == 0) {
            set_status(STATUS_FOUND);

            printf("\n======= A CHILD HAS FOUND THE DESTINATION URL, WAITING FOR OTHER PROCESSES TO TERMINATE =======\n");
            char *final_path[max_depth + 2];
            for (int j = 0; j < path_len; j++) {
                final_path[j] = path[j];
            }
            final_path[path_len] = urls[i];

            sprint_path(best_path_str, final_path, path_len + 1);
            strcpy(found_path_str_out, best_path_str);

            free_hyperlinks(labels, urls, num_links);
            return depth + 1;
        }
    }

    // Check if the current depth has reached the max, and should not proceed
    if (depth >= max_depth - 1) {
        printf("Max depth of %d reached.\n", max_depth);
        free_hyperlinks(labels, urls, num_links);
        return -1;
    }

    // Creates pipes and array to store children pids to wait later
    int pipes[num_links][2];
    int children_pids[num_links];
    int num_children = 0;

    // Loop through all neighbors
    for (int i = 0; i < num_links; i++) {
        if (should_terminate()) {
            break;
        }

        usleep(RATELIMITER_TIME);

        // Wait until the max process limit is no longer an issue (ie a spot has freed up)
        while (!process_limit_check(max_processes)) {
            if (should_terminate()) {
                break;
            }

            printf("Processes: (%d/%d), waiting for a child to finish...\n", max_processes, max_processes);

            if (num_children == 0) {
                usleep(200000);
                continue;
            }

            int child_result = -1;

            waitpid(children_pids[num_children - 1], NULL, 0);

            if (read(pipes[num_children - 1][0], &child_result, sizeof(int)) != sizeof(int)) {
                perror("read from child");
                child_result = -1;
            }
            
            if (child_result != -1) {
                char temp_path[MAX_PATH_LEN];
                uint32_t len = 0;
                read_full(pipes[num_children - 1][0], &len, sizeof(uint32_t));
                if (len > 0 && len <= MAX_PATH_LEN) {
                    read_full(pipes[num_children - 1][0], temp_path, len);
                    temp_path[len - 1] = '\0';
                    if (best_result == -1 || child_result < best_result) {
                        best_result = child_result;
                        strcpy(best_path_str, temp_path);
                    }
                }
            }
            close(pipes[num_children - 1][0]);

            if (child_result != -1) {
                set_status(STATUS_FOUND);
                break;
            }

            num_children--;
        }

        if (should_terminate()) {
            break;
        }

        if (check_and_add_discovered_url(urls[i])) {
            printf("Skipping: %s\n", urls[i]);
            continue;
        } else {
            printf("Found: %s\n", urls[i]);
        }

        if (pipe(pipes[num_children]) == -1) {
            perror("pipe");
            process_end();
            free_hyperlinks(labels, urls, num_links);
            return -1;
        }

        // Fork the children

        int r = fork();

        if (r < 0) {
            perror("fork");
            close(pipes[num_children][0]);
            close(pipes[num_children][1]);
            process_end();
            free_hyperlinks(labels, urls, num_links);
            return -1;
        }

        if (r == 0) {
            close(pipes[num_children][0]);

            if (should_terminate()) {
                int result = -1;
                write(pipes[num_children][1], &result, sizeof(int));
                close(pipes[num_children][1]);
                free_hyperlinks(labels, urls, num_links);
                process_end();
                exit(0);
            }

            printf("Child searching at depth %d\n", depth + 1);

            char *next_path[max_depth + 2];
            for (int j = 0; j < path_len; j++) {
                next_path[j] = path[j];
            }
            next_path[path_len] = urls[i];

            char child_path_str[MAX_PATH_LEN] = "";
            int result = depth_first_search(urls[i], end_url, depth + 1, max_depth, max_processes, next_path, path_len + 1, child_path_str);

            if (result != -1) {
                set_status(STATUS_FOUND);
            }

            if (write(pipes[num_children][1], &result, sizeof(int)) != sizeof(int)) {
                perror("write");
                close(pipes[num_children][1]);
                free_hyperlinks(labels, urls, num_links);
                process_end();
                exit(1);
            }
            
            if (result != -1) {
                uint32_t len = strlen(child_path_str) + 1;
                write_full(pipes[num_children][1], &len, sizeof(uint32_t));
                write_full(pipes[num_children][1], child_path_str, len);
            }

            close(pipes[num_children][1]);
            free_hyperlinks(labels, urls, num_links);
            process_end();
            exit(0);
        }

        children_pids[num_children] = r;
        close(pipes[num_children][1]);
        num_children++;
    }

    // For every child that was spawned, wait for it, read from the pipe
    for (int i = 0; i < num_children; i++) {
        waitpid(children_pids[i], NULL, 0);
        int child_result = -1;
        if (read(pipes[i][0], &child_result, sizeof(int)) != sizeof(int)) {
            perror("read from child");
            child_result = -1;
        }

        if (child_result != -1) {
            char temp_path[MAX_PATH_LEN];
            uint32_t len = 0;
            read_full(pipes[i][0], &len, sizeof(uint32_t));
            if (len > 0 && len <= MAX_PATH_LEN) {
                read_full(pipes[i][0], temp_path, len);
                temp_path[len - 1] = '\0';
                if (best_result == -1 || child_result < best_result) {
                    best_result = child_result;
                    strcpy(best_path_str, temp_path);
                }
            }
            set_status(STATUS_FOUND);
        }
        close(pipes[i][0]);
    }

    // Free the space for the urls adn labels
    free_hyperlinks(labels, urls, num_links);
    
    if (best_result != -1) {
        strcpy(found_path_str_out, best_path_str);
    }
    return best_result;
}

// Start the recursive DFS search
void start_search(char *start_url, char *end_url, int max_depth, int max_processes){
    initialize_urls_file();
    initialize_processes_file();
    initialize_status_file();
    cleanup_html_files();
    char *path[4];
    path[0] = start_url;
    char found_path_str[MAX_PATH_LEN] = "";
    int result = depth_first_search(start_url, end_url, 0, max_depth + 1, max_processes, path, 1, found_path_str);
    if (result == -1){
        int status = read_status();
        if (status == STATUS_PROCESS_CAP) {
            printf("\n ======= Search terminated: process cap of %d was reached. ======= \n", max_processes);
        } else {
            printf("\nNo path found.\n");
        }
    }
    else {
        printf("%s\n", found_path_str);
        printf("Path Length: %d\n", result);
    }
    cleanup_html_files();
}
