#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINKS 1000
#define MAX_URL_LEN 100

int main(void){
    return 0;
}

int extract_hyperlinks(char *url, char ***labels_out, char ***links_out){
    return -1;
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
    int num_links = extract_hyperlinks(current_url, &labels, &urls);
    if (num_links <= 0) {
        return -1;
    }
    // Creates pipe fds for children -> parent communication
    int pipes[MAX_LINKS][2];
    int children_pids[MAX_LINKS];
    int num_of_children = 0;
    // Loop through every neighbour of the current article
    for (int i = 0; i < num_links; i++){
        // TODO: Check if we have exceeded the max number of processes. If so, quit. If not, increment
        // TODO: Check if the url is in the file (meaning it already has been seen). If so, quit. If not, continue
        // Check if the url is the right one. If so, directly return the current depth
        if (strcmp(end_url, urls[i]) == 0){
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
                result = child_result;
            }
        }
        close(pipes[i][0]);
        waitpid(children_pids[i], NULL, 0);
    }
    free_hyperlinks(labels, urls, num_links);
    return result;
}
