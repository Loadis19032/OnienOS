#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pros.h>
#include "../../drivers/power/power.h"
#include "../../drivers/keyboard/keyboard.h"

#define MAX_ARGS 16
#define MAX_ARG_LENGTH 64

static int parse_args(char* input, int* argc, char argv[MAX_ARGS][MAX_ARG_LENGTH]) {
    char* token;
    int count = 0;
    
    if (input == NULL || argc == NULL || argv == NULL) {
        return -1;
    }
    
    size_t len = strlen(input);
    if (len > 0 && (input[len-1] == '\n' || input[len-1] == '\r')) {
        input[len-1] = '\0';
    }
    
    token = strtok(input, " \t");
    
    while (token != NULL && count < MAX_ARGS) {
        size_t token_len = strlen(token);
        
        if (token_len >= MAX_ARG_LENGTH) {
            token_len = MAX_ARG_LENGTH - 1;
        }
        
        strcpy(argv[count], token);
        count++;
        
        token = strtok(NULL, " \t");
    }
    
    *argc = count;
    return 0;
}

void handle_command(char* input) {
    int argc;
    char argv[MAX_ARGS][MAX_ARG_LENGTH];
    
    if (parse_args(input, &argc, argv) == 0 && argc > 0) {
        if (strcmp(argv[0], "shutdown") == 0) {
            shutdown();
        }
        else if (strcmp(argv[0], "reboot") == 0) {
            reboot();
        }
        else if (strcmp(argv[0], "help") == 0) {
            printf("Available commands:\n");
            printf("  shutdown - Turn off computer\n");
            printf("  reboot   - Restart computer\n");
            printf("  help     - Show this help\n");
            printf("  clear    - clear screen\n\n");
            printf("  touch    - create file (2 argv - path)\n");
            printf("  mkdir    - create directory (2 argv - path)\n");
            printf("  edit     - write file (2 argv - path, 3 argv - data)\n");
            printf("  cat      - read file (2 argv - path)\n");
            printf("  ls       - listing directory (2 argv - path)\n");
            printf("  fsinfo   - file system info\n");
        }
        else if (strcmp(argv[0], "clear") == 0) {
            clear();
        }
        else {
            printf("Unknown command: %s\n", argv[0]);
            printf("Type 'help' for available commands\n");
        }
    }
}

int shell() {   
    ata_device_t *dev = get_ata_device(0);



    char input[256];

    while (1) {
        printf("> ");
        fgets(input, 256);

        handle_command(input);
    }

    return 0;
}