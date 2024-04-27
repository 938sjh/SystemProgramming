/* $begin shellmain */
#include "myshell.h"

Job_t *jobs = NULL;
int job_count = 0;
int next_job = 1;

sigset_t prev_mask, mask;

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    char cwd[MAXLINE];
    
    Signal(SIGINT, sigint_handler);
    Signal(SIGTSTP, sigtstp_handler);
    Signal(SIGCHLD, sigchld_handler);

    //job initialize
    initialize();

    Sigemptyset(&mask);
    Sigaddset(&mask, SIGTSTP);
    Sigaddset(&mask, SIGINT);

    while (1) {
	/* Read */
    getcwd(cwd, MAXLINE);
	printf("%s> ", cwd);                   
	fgets(cmdline, MAXLINE, stdin); 
	if (feof(stdin))
	    exit(0);

	/* Evaluate */
	eval(cmdline);
    usleep(100000);
    } 
}
/* $end shellmain */

