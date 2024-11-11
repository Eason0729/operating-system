#include "sender.h"
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <sys/msg.h>
#include <time.h>
#define assert(x, msg)                                                         \
  {                                                                            \
    if (!(x)) {                                                                \
      fprintf(stderr, "%s\n", msg);                                            \
      perror("Error");                                                         \
      exit(1);                                                                 \
    }                                                                          \
  }

void send(message_t message, mailbox_t *mailbox_ptr) {
  /*  TODO:
      1. Use flag to determine the communication method
      2. According to the communication method, send the message
  */
  if (mailbox_ptr->flag == 1) {
    assert(msgsnd(mailbox_ptr->storage.msqid, message.message,
                  sizeof(char) * strnlen((char *)&message.message[0], 1024),
                  0) != -1,
           "fail sending message");
  } else if (mailbox_ptr->flag == 2) {
    shm_map *shm_addr = mailbox_ptr->storage.shm_addr;
    size_t len = strnlen(message.message, 1024);
    for (size_t i = 0; i < len; i++)
      shm_addr->messages[shm_addr->end][i] = message.message[i];
    message.message[len] = '\0';
    shm_addr->end++;
  }
}

double time_taken = 0.0;
void start_clock(struct timespec *start) {
  clock_gettime(_POSIX_MONOTONIC_CLOCK, start);
}
void end_clock(struct timespec *start) {
  struct timespec end;
  clock_gettime(_POSIX_MONOTONIC_CLOCK, &end);
  time_taken +=
      (end.tv_sec - start->tv_sec) + (end.tv_nsec - start->tv_nsec) * 1e-9;
}

int main(int argc, char **argv) {
  /*  TODO:
      1) Call send(message, &mailbox) according to the flow in slide 4
      2) Measure the total sending time
      3) Get the mechanism and the input file from command line arguments
          • e.g. ./sender 1 input.txt
                  (1 for Message Passing, 2 for Shared Memory)
      4) Get the messages to be sent from the input file
      5) Print information on the console according to the output format
      6) If the message form the input file is EOF, send an exit message to the
     receiver.c 7) Print the total sending time and terminate the sender.c
  */

  mailbox_t mailbox;
  assert(argc == 3, "usage: (1|2) <file>");
  assert(argv[1][0] == '1' || argv[1][0] == '2', "expect mode 1 or 2");
  if (argv[1][0] == '1') {
    printf("\033[1;34mmessage passing\033[0m\n");
    mailbox.flag = 1;
    mailbox.storage.msqid = msgget(1145, 0666 | IPC_CREAT);
  } else {
    printf("\033[1;34mshare memory\033[0m\n");
    mailbox.flag = 2;
    int shmid = shmget(1145, sizeof(shm_map), IPC_CREAT | 0666);
    mailbox.storage.shm_addr = shmat(shmid, NULL, 0);
    mailbox.storage.shm_addr->start = 0;
    mailbox.storage.shm_addr->end = 0;
  }

  struct timespec start;

  FILE *file = fopen(argv[2], "r");
  assert(file != NULL, "fail opening file");

  sem_unlink("/os-lab1-send");
  sem_unlink("/os-lab1-rec");
  sem_t *sem_send = sem_open("/os-lab1-send", O_CREAT | O_EXCL, 0777, 1);
  sem_t *sem_rec = sem_open("/os-lab1-rec", O_CREAT | O_EXCL, 0777, 0);
  assert(sem_send != NULL, "fail opening semaphore");
  assert(sem_rec != NULL, "fail opening semaphore");

  while (!feof(file)) {
    message_t message;
    size_t len = 1025;

    char *line_ptr = message.message;

    size_t read = getline(&line_ptr, &len, file);
    if (message.message[read - 1] == '\n')
      message.message[read - 1] = '\0';
    else
      message.message[read] = '\0';
    sem_wait(sem_send);
    start_clock(&start);
    printf("\033[1;34mSending Message:\033[0m %s\n", line_ptr);
    send(message, &mailbox);
    end_clock(&start);
    sem_post(sem_rec);
  }

  message_t message;
  message.message[0] = 'e';
  message.message[1] = 'x';
  message.message[2] = 'i';
  message.message[3] = 't';
  message.message[4] = '\0';

  sem_wait(sem_send);
  send(message, &mailbox);
  sem_post(sem_rec);
  sem_post(sem_rec);
  sem_post(sem_send);

  printf("\033[0;31mSender exit\033[0;0m\n");

  printf("Total time taken sending msg: %1.6f s\n", time_taken);
}
