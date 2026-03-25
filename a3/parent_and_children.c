#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

#define MAX_STR_LEN 100
#define MAX_HYPERLINKS 1000

typedef struct HyperlinkLL{
    char label[MAX_STR_LEN];
    int occurrences;
    struct HyperlinkLL *next;
} HyperlinkLL;

void create_hyperlink_and_pipe(char *hyperlink_label, int num_of_hyperlinks, int result_pipes[]){
    // Create new hyperlink object
    HyperlinkLL h;
    strcpy(h.label, hyperlink_label);
    h.occurrences = num_of_hyperlinks;
    h.next = NULL;
    // Create pipe
    int parent_to_child[2]; // Parent writing to child
    int child_to_parent[2]; // Child writing to parent
    if (pipe(parent_to_child) == -1){
        perror("pipe");
        return;
    }
    if (pipe(child_to_parent) == -1){
        perror("pipe");
        return;
    }
    // Fork
    int r = fork();
    if (r < 0){
        perror("fork");
        return;
    }
    // Child process
    else if (r == 0){
        // Close the necessary ends
        close(parent_to_child[1]); // Close writing to first pipe
        close(child_to_parent[0]); // Close reading to second pipe
        // Read and change info
        HyperlinkLL h_child;
        read(parent_to_child[0], &h_child, sizeof(HyperlinkLL));
        printf("Child received hyperlink: %s , %d occurrences. \n", h_child.label, h_child.occurrences);

        // TODO: using the current fp and cursor location, find the number of subsequent labels;
        int num_of_occurrences = 100;

        h_child.occurrences = num_of_occurrences;
        write(child_to_parent[1], &h_child, sizeof(HyperlinkLL));
        // Close the ends when finished with them
        close(parent_to_child[0]);
        close(child_to_parent[1]);
        exit(0);
    }
    // Parent process
    else {
        // Close the necessary ends
        close(parent_to_child[0]); // Close reading to first pipe
        close(child_to_parent[1]); // Close writing to second pipe
        // Write info to child
        write(parent_to_child[1], &h, sizeof(HyperlinkLL));
        close(parent_to_child[1]);
        // Save pipe to be used later
        result_pipes[num_of_hyperlinks] = child_to_parent[0];
    }
}

HyperlinkLL *get_hyperlinkLL_from_file(FILE *fp){
    int num_of_hyperlinks = 0;
    int result_pipes[MAX_HYPERLINKS];
    HyperlinkLL *hyperlinks_results = NULL;
    HyperlinkLL *hyperlinks_last = NULL;

    // TODO: Insert code that parses through the file, and each time a hyperlink is found, assign found_label to be the label,
    // Then, call create_hyperlink_and_pipe()

    while (num_of_hyperlinks < 100) { // Simulate 100 unique labels found
        char *found_label = "label";
        create_hyperlink_and_pipe(found_label, num_of_hyperlinks, result_pipes);
        num_of_hyperlinks++;
    }

    for (int i = 0; i < num_of_hyperlinks; i++) {
        wait(NULL);
    }

    for (int j = 0; j < num_of_hyperlinks; j++) {
        HyperlinkLL *h = malloc(sizeof(HyperlinkLL));
        read(result_pipes[j], h, sizeof(HyperlinkLL));
        h->next = NULL;

        if (hyperlinks_results == NULL){
            hyperlinks_results = h;
            hyperlinks_last = h;
        }
        else {
            hyperlinks_last->next = h;
            hyperlinks_last = h;
        }
        printf("Parent reads hyperlinks: %s, %d occurrences. \n", h->label, h->occurrences);
        close(result_pipes[j]);
    }
    return hyperlinks_results;
}

int main(int argc, char **argv) {
    FILE *fp = get_html_fp(argv[1]);
    HyperlinkLL *h = get_hyperlinkLL_from_file(fp);
    printf("Hyperlink linked lists returned. \n");
    while (h-> next != NULL){
        printf("Hyperlink: %s, %d occurrences. \n", h->label, h->occurrences);
        h = h->next;
    }
    return 0;
}
