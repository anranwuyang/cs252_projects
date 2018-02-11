#include "command.hh"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
int yyparse(void);

extern "C" void disp( int sig )
{
	fprintf( stderr, "\nsig:%d      Ouch!\n", sig);
	Command::_currentCommand.prompt();
}

extern "C" void dispZombie(int sig){
    int pid = wait3(0, 0, NULL);
    while(waitpid(-1, NULL, WNOHANG) > 0);
    /*if(pid > 0){
        fprintf( stderr, "\n[%d] exited.\n", pid);
        Command::_currentCommand.prompt();
    }*/
}
int main() {
	struct sigaction sa;
    sa.sa_handler = disp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGINT, &sa, NULL)){
        perror("sigaction");
        exit(2);
    }

    struct sigaction killZombie;
    killZombie.sa_handler = dispZombie;
    sigemptyset(&killZombie.sa_mask);
    killZombie.sa_flags = SA_RESTART;
   
     if (sigaction(SIGCHLD, &killZombie, NULL)){
        perror("sigaction");
        exit(2);
    }

	Command::_currentCommand.prompt();
	yyparse();
}
