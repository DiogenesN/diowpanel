/* Runs the given command with arguments
 * also supports quoted arguments but you need to build the command with \"%s\"
 * You need to provide the full path to the executable
 * If you don't provide the full path then it will look in /usr/bin and /usr/local/bin
 * Usage:
 * char *cmd = "/usr/bin/xfce4-terminal --title=TEST --hide-borders --hide-menubar --hide-scrollbar";
 * run_cmd(cmd);
 */

#include <spawn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ARGS 300

int iterCount = 0;
char addPath[2048];
extern char **environ;
static int currChar = 0;
static bool isUsrBin = true;

char *process_arguments(char **command_p) {
	// Trying to guess the path if not provided
	if (isUsrBin && iterCount == 0 && command_p[0][0] != '/') { // path to 
		snprintf(addPath, sizeof(addPath), "/usr/local/bin/%s", *command_p);
		*command_p = addPath;
		iterCount = iterCount + 1;
	}
	else if (!isUsrBin && iterCount == 1 && command_p[0][0] != '/') {
		snprintf(addPath, sizeof(addPath), "/usr/bin/%s", *command_p);
		*command_p = addPath;
		iterCount = 2;
	}

	char *text_p = *command_p;
	char tmp[2048];
	int count = 0;
	int tmp_index = 0;
	bool inQuotes = false;

	while (text_p[count] != '\0') {
		if (text_p[count] == '"' && !inQuotes) {
			inQuotes = true;
		}
		else if (text_p[count] == '"' && inQuotes) {
			inQuotes = false;
		}
		else {
			if (inQuotes && text_p[count] == ' ') {
				tmp[tmp_index++] = ' ';
			}
			else {
				tmp[tmp_index++] = text_p[count];
			}
		}
		count++;
		if (!inQuotes && text_p[count] == ' ') {
			break;
		}
	}

	tmp[tmp_index] = '\0';
	currChar = text_p[count];

	*command_p = &text_p[count + (currChar != '\0')]; // Move the pointer forward
	return strdup(tmp); // Return a new copy of the string
}

void run_cmd(char *command) {
	pid_t pid;
	int argc = 0;
	char *argv[MAX_ARGS];
	char *command_p = command;

	/// Tokenize command and populate argv
	while (*command_p != '\0' && argc < MAX_ARGS) {
		char *string = process_arguments(&command_p);
		argv[argc] = string;
		argc = argc + 1;
	}

    // Null terminate the array
	argv[argc] = NULL;

	// Execute the command
	int status = posix_spawn(&pid, argv[0], NULL, NULL, argv, environ);

	if (status != 0 && iterCount < 2) {
		isUsrBin = false;
		run_cmd((char *)command);
	}
	else if (status != 0 && iterCount == 2) {
		fprintf(stderr, "Error: %s. Please provide full path to %s\n", strerror(status), argv[0]);
	}

    /// Print and free allocated memory
	for (int i = 0; argv[i] != NULL; i++) {
		///printf("argv[%d]: %s\n", i, argv[i]);
		free(argv[i]); // Free each string
	}
}
