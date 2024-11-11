#include "receiver.h"
#include <errno.h>
#include <sys/msg.h>
#include <time.h>
#define assert(x, msg)                                                         \
  {                                                                            \
    if (!(x)) {                                                                \
      fprintf(stderr, "%s\n", msg);                                            \
      exit(1);                                                                 \
    }                                                                          \
  }

void receive(message_t *message_ptr, mailbox_t *mailbox_ptr) {
  /*  TODO:
      1. Use flag to determine the communication method
      2. According to the communication method, receive the message
  */
  for (size_t i = 0; i < 1024; i++)
    message_ptr->message[i] = '\0';

  if (mailbox_ptr->flag == 1) {
    msgrcv(mailbox_ptr->storage.msqid, message_ptr->message, 1024, 0, 0);
  } else {
    shm_map *shm_addr = mailbox_ptr->storage.shm_addr;
    if (shm_addr->start != shm_addr->end) {
      char *output = shm_addr->messages[shm_addr->start++];
      size_t len = strnlen(output, 1024);
      for (size_t i = 0; i < len; i++)
        message_ptr->message[i] = output[i];
      output[len] = '\0';
    }
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
      1) Call receive(&message, &mailbox) according to the flow in slide 4
      2) Measure the total receiving time
      3) Get the mechanism from command line arguments
          • e.g. ./receiver 1
      4) Print information on the console according to the output format
      5) If the exit message is received, print the total receiving time and
     terminate the receiver.c
  */
  mailbox_t mailbox;
  assert(argc == 2, "usage: (1|2)");
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
  }

  struct timespec start;

  sem_t *sem_send = sem_open("/os-lab1-send", O_EXCL, 0777, 1);
  sem_t *sem_rec = sem_open("/os-lab1-rec", O_EXCL, 0777, 0);
  assert(sem_send != NULL, "fail opening semaphore");
  assert(sem_rec != NULL, "fail opening semaphore");

  while (errno == 0) {
    message_t message;

    sem_wait(sem_rec);
    start_clock(&start);
    receive(&message, &mailbox);
    end_clock(&start);
    sem_post(sem_send);

    if (strcmp(message.message, "exit") == 0)
      break;
    printf("\033[1;34mReceiving Message:\033[0m %s\n", message.message);
  }
  printf("\033[0;31mRecevier exit\033[0;0m\n");

  printf("Total time taken receving msg: %1.6f s\n", time_taken);
}
