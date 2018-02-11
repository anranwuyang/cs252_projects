
/*
 *
 * CS-252 Fall 2017
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include "command.hh"
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include "y.tab.hh"



static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

\$\{.\} {
  char * exp = strdup((&yytext[2]));
  char * ptr = exp;
  while(*ptr != '}'){
    ptr++;
  }
  *ptr = '\0';
  char * repl;
  if(strcmp(exp, "$") == 0){
    repl = (char *) malloc(sizeof(char) * 10);
    sprintf(repl, "%d", getpid());
  }else{
    repl = strdup(getenv(exp));
  }
  
  yylval.string_val = repl;
  return WORD;
}

(([\\][&<>"|\\])|[^ \t\n&<>"|])(([\\][&<>"|\\])|[^ \t\n&<>"|])* {
  /* Assume that file names have only alpha chars */
  yylval.string_val = (char*) malloc(sizeof(char) * 8192);
  char * ptr = yytext;
  char * valptr = yylval.string_val;
  while(*ptr != '\0'){
    if(*ptr == '$' && *(ptr +1) != '\0' && *(ptr +1) == '{'){
      ptr += 2;
      char * subptr = ptr;
      char * exp = (char *)malloc(sizeof(char) * 512);
      char * expPtr = exp;
      while(*subptr != '\0' && *subptr != '}'){
        *expPtr = *subptr;
        expPtr++;
        subptr++;
        ptr++;
      }
      *expPtr = '\0';
      ptr++;

      char * repl;
      if(strcmp(exp, "$") == 0){
        repl = (char *) malloc(sizeof(char) * 10);
        sprintf(repl, "%d", getpid());
      }else{
        repl = strdup(getenv(exp));
      }

      char * replPtr = repl;
      while(*replPtr != '\0'){
        *valptr = *replPtr;
        valptr++;
        replPtr++;
      }
    }else{
      if(*ptr == '\\'){
        ptr++;
      }
      *valptr = *ptr;
      valptr++;
      ptr++;
    }
  }
  *valptr = '\0';
  return WORD;
}

\".*\" {
	//Quote escaped string

  yylval.string_val = strdup((&yytext[1]));
  char * ptr = yylval.string_val;
  while(*ptr != '\"'){
  	ptr++;
  }
  *ptr = '\0';
  return STRING;
}

`.*` {
  //Quote escaped string

  char * cmd = strdup((&yytext[1]));
  char * ptr = cmd;
  while(*ptr != '`'){
    ptr++;
  }
  *ptr = '\0';
  strcat(cmd, "\nexit\n");

  //printf("%s\n", cmd);
  int tin = dup(0);
  int tout = dup(1);

  int toSub[2];
  int fromSub[2];
  pipe(toSub);
  pipe(fromSub);

  

  write(toSub[1], cmd, strlen(cmd));
  close(toSub[1]);
  //printf("Going to fork\n");
  dup2(toSub[0], 0);
  close(toSub[0]);
  dup2(fromSub[1], 1);
  close(fromSub[1]);

  
  int frk = fork();
  if(frk < 0){
    perror("Fork Fail");
    exit(2);
  }else if(frk == 0){
    
    char ** argvs = NULL;
    execvp("/proc/self/exe", argvs);
    _exit(1);
  }else{
    while(waitpid(frk, NULL, 0) == -1);
    dup2(tin, 0);
    dup2(tout, 1);
    close(toSub[0]);
    close(fromSub[1]);
    close(toSub[1]);
    //printf("Child proc compelted\n");
    char buf[32192];
    char * bufptr = buf;
    while(read(fromSub[0], bufptr, 1)){
      if(*bufptr == '\n'){
        *bufptr = ' ';
      }
      bufptr++;
    };
    *bufptr = '\0';
    close(fromSub[0]);
    char * ptr2 = buf + strlen(buf);
    

    while(ptr2 != buf){
      unput(*ptr2);
      ptr2--;
    }
    unput(*ptr2);
  }  
}



">" {
  return GREAT;
}

">>" {
	return GREATGREAT;
}

"|" {
	return PIPE;
}

"<" {
	return LESS;
}

"2>" {
	return TWOGREAT;
}

">>&" {
	return GREATGREATAMPERSAND;
}

">&" {
	return GREATAMPERSAND;
}

"&" {
	return AMPERSAND;
}

. {
	return NOTOKEN;
}

%%
