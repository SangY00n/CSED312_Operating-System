#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct thread* get_child_process(int pid);
char* parse_file_name(char* s);
int parse_for_arguments (char **argv, int argc, char *s);
void stack_argument_init(char **argv, int argc, void **esp);


#endif /* userprog/process.h */
