#include <stdlib.h>
#include "include/shell.h"
#include "include/command.h"
#include "include/greeter.h"

int history_count;
char *history[MAX_RECORD_NUM];

int main(int argc, char *argv[]) {
	greet();

	for (int i = 0; i < MAX_RECORD_NUM; ++i)
		history[i] = (char *) malloc(BUF_SIZE * sizeof(char));

	shell();

	for (int i = 0; i < MAX_RECORD_NUM; ++i)
		free(history[i]);

	return 0;
}