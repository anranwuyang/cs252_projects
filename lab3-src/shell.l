
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
  #include <regex>

#include "command.hh"
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include "y.tab.hh"
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

////////////  Start added code ///////////

extern "C" char * read_line();

int mygetc(FILE * f) {
static char *p;
char ch;

if (!isatty(0)) {
 // stdin is not a tty. Call real getc
 return getc(f);
}

// stdin is a tty. Call our read_line.

if (p==NULL || *p == 0) {
 char * s = read_line();
 p = s;
}

ch = *p;
p++;

return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////

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

(([\\][&<>"|\\ ])|[^ \t\n&<>"|])(([\\][&<>"|\\ ])|[^ \t\n&<>"|])* {
  /* Assume that file names have only alpha chars */
  // (char*) malloc(sizeof(char) * 8192);
  std::string matchW (strdup(yytext));

  std::regex escpR ("\\\\([&<>\"|\\\\ ])");

  matchW = std::regex_replace (matchW,escpR,"$1");

  std::regex pidExpan ("\\$\\{\\$\\}");
  char * pid= (char *) malloc(sizeof(char) * 10);
  sprintf(pid, "%d", getpid());
  matchW = std::regex_replace (matchW,pidExpan,pid);
  free(pid);

  std::regex statusExpan ("\\$\\{\\?\\}");
  char * lastExit= (char *) malloc(sizeof(char) * 10);
  sprintf(lastExit, "%d", WEXITSTATUS(Command::_lastStatus));
  matchW = std::regex_replace (matchW,statusExpan,lastExit);
  free(lastExit);
  
  std::regex lastArgExpan ("\\$\\{_\\}");
  matchW = std::regex_replace (matchW,lastArgExpan,Command::_lastArg);

  std::regex backPidExpan("\\$\\{!\\}");
  char * pidBack = (char *) malloc(sizeof(char) * 10);
  if(Command::_lastPid != 0){
		sprintf(pidBack, "%d", Command::_lastPid);
  }else{
    sprintf(pidBack, "");
  }
	matchW=  std::regex_replace(matchW, backPidExpan, pidBack);
  free(pidBack);

  std::regex shellExpan ("\\$\\{SHELL\\}");
  char * path= (char *) malloc(sizeof(char) * 1048);
  readlink("/proc/self/exe", path, 1048 );
  matchW = std::regex_replace (matchW,shellExpan,path);
  free(path);

  std::regex tildeExpan ("^~$");
  matchW = std::regex_replace (matchW,tildeExpan,std::string(getenv("HOME")));

  std::regex tildeExpan2 ("^~([A-Za-z0-9]*)");
  std::smatch tilMatch;

  //printf("%s\n", "Searching");
  while ( regex_search(matchW, tilMatch, tildeExpan2))
  {
      //printf("match: %s\n", std::string(tilMatch[0]).c_str() );
      std::string token = std::string(tilMatch[0]);
      std::string username = token.substr(1,token.size()-1);

      struct passwd *pw =getpwnam(username.c_str());
      const char *homedir = pw->pw_dir;

      std::regex orig (std::string("^~") + username);

      matchW = std::regex_replace (matchW,orig, std::string(homedir));
  }
  

  //printf("Word: %s\n", matchW.c_str());
  std::regex envExpan;
  try {
     envExpan = std::regex ("(^|)[$][{][a-zA-Z0-9]+[}]($|)", std::regex::nosubs);
  } catch (std::regex_error& e) {
      printf("Regex error code: %d\n", e.code());
  }
  std::smatch sm;

  /*std::sregex_iterator iter(matchW.begin(), matchW.end(), envExpan, std::regex_constants::match_continuous);
  std::sregex_iterator end;
  while(iter != end)
    {
        for(unsigned i = 0; i < iter->size(); ++i)
        {
            printf("match: %s\n", std::string((*iter)[i]).c_str());
            std::string token = std::string((*iter)[i]);
            std::string var = token.substr(2,token.size()-3);

            std::regex orig (std::string("[$][{]") + var +std::string("[}]"));

            matchW = std::regex_replace (matchW,orig, getenv(var.c_str()));
            printf("After Replacement: %s\n", matchW.c_str());
        }
        ++iter;
    }*/
  
  while ( regex_search(matchW, sm, envExpan))
  {
        
      std::string token = std::string(sm[0]);
      std::string var = token.substr(2,token.size()-3);

      std::regex orig (std::string("[$][{]") + var +std::string("[}]"));

      matchW = std::regex_replace (matchW,orig, getenv(var.c_str()));
  }
  /*
  if (regex_search(matchW, sm, envExpan, std::regex_constants::match_continuous))
  {
      printf("Found\n");
      for (int i=0; i<sm.size(); i++)
      {   
        
      }
  }*/
  //std::regex envExpan ("\\$\\{(.)\\}");
  //matchW = std::regex_replace(matchW, envExpan, );
  /*
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
  }*/

  yylval.string_val = strdup(matchW.c_str());
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
