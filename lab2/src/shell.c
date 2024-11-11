#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/command.h"
#include "../include/builtin.h"

// ===============================================================

// ======================= requirement 2.2 =======================
/**
 * @brief
 * Execute external command
 * The external command is mainly divided into the following two steps:
 * 1. Call "fork()" to create child process
 * 2. Call "execvp()" to execute the corresponding executable file
 * @param p cmd_node structure
 * @return int
 * Return execution status
 */
int spawn_proc(struct cmd_node *p) {
	int status;

	char *path = p->args[0];

	pid_t child_pid = fork();
	if (child_pid == 0) {
		execvp(path, p->args);
		printf("Error: %s\n", strerror(errno));
		exit(1);
	}

	waitpid(child_pid, &status, 0);

	if (status == 0) return 1;
	else return -1;
}

/**
 * @brief
 * execute any command, and handle fd(inherit, close, dup, dup2)
 *
 * Note that it leak resource(WHO CARE?)
 *
 * @param cmd Command structure
 * @param int input
 * @param out output
 * @return execution status
 */
int execCommand(struct cmd_node *cmd, int in, int out) {
	int o_in = dup(STDIN_FILENO), o_out = dup(STDOUT_FILENO);
	if (o_in == -1 | o_out == -1)
		perror("dup");

	dup2(in, STDIN_FILENO);
	dup2(out, STDOUT_FILENO);

	int command = searchBuiltInCommand(cmd);

	int status;

	if (command == -1)
		status = spawn_proc(cmd);
	else status = execBuiltInCommand(command, cmd);

	dup2(o_in, 0);
	dup2(o_out, 1);

	return status;
}

// ===============================================================


// ======================= requirement 2.4 =======================
/**
 * @brief
 * Use "pipe()" to create a communication bridge between processes
 * Call "spawn_proc()" in order according to the number of cmd_node
 * @param cmd Command structure
 * @return int
 * Return execution status
 */
int fork_cmd_node(struct cmd *cmd) {
	struct cmd_node *current = cmd->head;
	size_t n_exec = 0;

	int status = 0;

	int read_pipe = STDIN_FILENO;

	while (current && n_exec < 64) {
		int pipes[2];

		pipe(pipes);

		if(current->next==NULL)status = execCommand(current, read_pipe, STDOUT_FILENO);
		else status = execCommand(current, read_pipe, pipes[1]);

		close(pipes[1]);
		read_pipe = pipes[0];

		current = current->next;
	}

	return status;
}

// ===============================================================

void shell() {
	while (1) {
		printf(">>> $ ");
		char *buffer = read_line();
		if (buffer == NULL)
			continue;

		struct cmd *cmd = split_line(buffer);

		int status = -1;
		// only a single command
		struct cmd_node *temp = cmd->head;

		if (temp->next == NULL) {
			int in = STDIN_FILENO, out = STDOUT_FILENO;
			if (temp->in_file) in = open(temp->in_file, O_RDONLY);

			if (temp->out_file) out = open(temp->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

			if (in == -1 || out == -1) {
				perror("open");
				continue;
			}

			status = execCommand(cmd->head, in, out);
		}
		// There are multiple commands ( | )
		else {
			status = fork_cmd_node(cmd);
		}
		// free space
		while (cmd->head) {
			struct cmd_node *temp = cmd->head;
			cmd->head = cmd->head->next;
			free(temp->args);
			free(temp);
		}
		free(cmd);
		free(buffer);

		if (status == 0)
			break;
	}
}
