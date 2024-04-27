/* $begin shellmain */
#include "myshell.h"

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    char cwd[MAXLINE];
    
    Signal(SIGINT, sigint_handler);
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

