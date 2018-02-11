
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>
  #include <string.h>

#include <sys/types.h>
#include <regex.h>
  #include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
  #include <vector>
#include <algorithm> 
  #include <unistd.h>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <string_val> WORD STRING SUBSHELL
%token NOTOKEN GREAT NEWLINE PIPE AMPERSAND GREATGREAT LESS GREATAMPERSAND GREATGREATAMPERSAND TWOGREAT END_OF_FILE

%{
//#define yylex yylex
#include <cstdio>
#include "command.hh"

void expandWildCards(char * path);
void expandWildCard(char * prefix, char * suffix, std::vector<std::string> &res);
void yyerror(const char * s);
int yylex();

%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command: 
  NOTOKEN {
    //printf("NOTOKEN DETECTED\n");
  }
  | command_and_args pipe_list iomodifiers background_opt NEWLINE {
    ////printf("   Yacc: Execute command.\n");
    if(Command::_lastArg != NULL){
      free(Command::_lastArg);
    }
    Command::_lastArg = strdup(Command::_currentSimpleCommand->_arguments[Command::_currentSimpleCommand->_numOfArguments -1]);
    
    Command::_currentCommand.execute();
   

  }
  | NEWLINE{
    Command::_currentCommand.prompt();
  }
  | error NEWLINE { yyerrok; }
  ;

background_opt:
  AMPERSAND {
    Command::_currentCommand._background = 1;
  }
  |{
     Command::_currentCommand._background = 0;
  }
  ;

pipe_list:
  PIPE command_and_args pipe_list
  |
  ;

command_and_args:
  command_word argument_list {
    Command::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

argument_list:
  argument_list argument
  |
  ;

argument:
  WORD {
    //printf("   Yacc: insert argument \"%s\"\n", $1);
    if(*$1 != '\0'){
      expandWildCards($1);
      
    }
    
  }
  | STRING {
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

command_word:
  WORD {
    //printf("   Yacc: insert command \"%s\"\n", $1);
    
    if ( !strcmp( $1, "exit" ) ) {
      //printf( "Bye!\n");
      for (int i = 0; i < 1024; ++i) close(i);
      exit( 0 );
    }else{
       Command::_currentSimpleCommand = new SimpleCommand();
       Command::_currentSimpleCommand->insertArgument( $1 );
    }
  }
  ;

iomodifiers:
  iomodifier iomodifiers
  |
  ;

iomodifier:
  GREAT WORD {
    
    if(Command::_currentCommand._outFile || Command::_currentCommand._errFile){
      yyerror("Ambiguous output redirect");
    }else{
      //printf("   Yacc: insert stdout \"%s\"\n", $2);
      Command::_currentCommand._outFile = $2;
    }
    
  }
  | TWOGREAT WORD {
    
    if(Command::_currentCommand._outFile || Command::_currentCommand._errFile){
      yyerror("Ambiguous output redirect");
    }else{
      //printf("   Yacc: insert stderr \"%s\"\n", $2);
      Command::_currentCommand._errFile = $2;
    }
    
  }
  | GREATGREAT WORD {
    
    if(Command::_currentCommand._outFile || Command::_currentCommand._errFile){
      yyerror("Ambiguous output redirect");
    }else{
      //printf("   Yacc: append stdout \"%s\"\n", $2);
      Command::_currentCommand._outFile = $2;
      Command::_currentCommand._append = 1;
    }
  }
  | GREATAMPERSAND WORD {
    
    if(Command::_currentCommand._outFile || Command::_currentCommand._errFile){
      yyerror("Ambiguous output redirect");
    }else{
      //printf("   Yacc: insert stdout and stderr \"%s\"\n", $2);
      Command::_currentCommand._outFile = $2;
      Command::_currentCommand._errFile = $2;
    }
  }
  | GREATGREATAMPERSAND WORD {
    
    if(Command::_currentCommand._outFile || Command::_currentCommand._errFile){
      yyerror("Ambiguous output redirect");
    }else{
      //printf("   Yacc: append stdout and stderr \"%s\"\n", $2);
      Command::_currentCommand._outFile = $2;
      Command::_currentCommand._errFile = $2;
      Command::_currentCommand._append = 1;
    }
  }
  | LESS WORD {
    //printf("   Yacc: stdin \"%s\"\n", $2);
    Command::_currentCommand._inFile = $2;
  }
  ;

%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

void expandWildCards(char * path){
  if(strchr(path, '*') == NULL && strchr(path, '*') == NULL){
    Command::_currentSimpleCommand->insertArgument(path);
    return;
  }


  // /vector<int> *results = new vector<int>();
  std::vector<std::string> results;
  

  
  if(path[0] == '/'){
    expandWildCard(strdup("/"), strdup(path+1), results);
  }else{
    expandWildCard(strdup(""), strdup(path), results);
  }
  
  
  
  //printf("Wild card completed with %d results\n", results.size());
  std::sort(results.begin(), results.end());
  for(int i = 0; i < results.size(); i++){
    Command::_currentSimpleCommand->insertArgument(strdup(results[i].c_str()));
  }
}

void expandWildCard(char * prefix, char * suffix, std::vector<std::string> &res){
  if(*suffix == '\0'){
    //printf("Found!\n");
    res.push_back(std::string(prefix));
    return;
  }

  char * s = strchr(suffix, '/');
  char buf[2048];
  if(s != NULL){
    strncpy(buf, suffix, s - suffix);
    suffix = s+1;
  }else{
    strcpy(buf, suffix);
    suffix += strlen(suffix);
  }

  char newPrefix[2048];

  if(strchr(buf, '*') == NULL && strchr(buf, '?') == NULL){
    
    if(strlen(prefix) == 0 || (strlen(prefix) ==1 && prefix[0] == '/')){
      sprintf(newPrefix, "%s%s", prefix, buf);
    }else{
      sprintf(newPrefix, "%s/%s", prefix, buf);
    }
    expandWildCard(newPrefix, suffix, res);
    return;
  }

  char * reg = (char*)malloc(2*strlen(buf)+10);
  char * a = buf;
  char * r = reg;
  *r = '^'; r++; // match beginning of line
  while (*a) {
    if (*a == '*') { *r='.'; r++; *r='*'; r++; }
    else if (*a == '?') { *r='.'; r++;}
    else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
    else { *r=*a; r++;}
    a++;
  }
  *r='$'; r++; *r=0;

  regex_t rbin;
  regcomp(&rbin, reg, 0);

  DIR * d;
  if(strlen(prefix) == 0){
    d = opendir("."); 
  }else{
    d = opendir(prefix); 
  }
  if(d==NULL) return;

  struct dirent * ent;
  while((ent = readdir(d)) != NULL){
    regmatch_t match;
    if(regexec(&rbin, ent->d_name, 1, &match, 0) == 0){
      if(ent->d_name[0] == '.'){
        if(buf[0] == '.'){
          if(strlen(prefix) == 0 || (strlen(prefix) ==1 && prefix[0] == '/')){
            sprintf(newPrefix, "%s%s", prefix, ent->d_name);
          }else{
            sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
          }
          expandWildCard(newPrefix, suffix, res);
        }
      }else{
         if(strlen(prefix) == 0 || (strlen(prefix) ==1 && prefix[0] == '/')){
            sprintf(newPrefix, "%s%s", prefix, ent->d_name);
          }else{
            sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
          }
        expandWildCard(newPrefix, suffix, res);
      }
    }
  }
  closedir(d);
  return;
}

#if 0


main()
{
   yyparse();
}
#endif
