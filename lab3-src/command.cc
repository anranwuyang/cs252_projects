/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include "command.hh"

int yyparse(void);
extern FILE * yyin;
Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void Command:: clear() {
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void Command::print() {
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void Command::execute() {
	
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	//print();

	int tin = dup(0);
	int tout = dup(1);
	int terr = dup(2);

	int in;
	if(_inFile){
		in = open(_inFile, O_RDONLY, 0);
		if(in < 0){
			perror("open");
			_exit(1);
		}
	}else{
		in = dup(tin);
	}

	int out;
	int err = dup(terr);
	pid_t frk;
	for(int i = 0; i < _numOfSimpleCommands; i++){
		dup2(in, 0);
		close(in);

		if(i+1 == _numOfSimpleCommands){
			if(_outFile && _append){
				out = open(_outFile, O_WRONLY|O_APPEND|O_CREAT, S_IRWXU);
			}else if(_outFile){
				out = open(_outFile, O_CREAT|O_WRONLY|O_TRUNC, 0664);
			}else{
				out = dup(tout);
			}

			if(_errFile == _outFile){
				err = dup(out);
			}else if(_errFile && _append){
				err = open(_errFile, O_WRONLY|O_APPEND|O_CREAT, S_IRWXU);
			}else if(_errFile && !_append){
				err = open(_errFile, O_CREAT|O_WRONLY|O_TRUNC, 0664);
			}else{
				err = dup(terr);
			}
			
		}else{
			int fpipe[2];
			pipe(fpipe);
			out = fpipe[1];
			in = fpipe[0];
		}

		dup2(out, 1);
		close(out);

		dup2(err, 2);
		close(err);

		if(strcmp(_simpleCommands[i]->_arguments[0], "source") == 0){
			if(_simpleCommands[i]->_numOfArguments < 2){
				perror("Too few arguments");
				exit(-1);
			}else{
				FILE *f = fopen(_simpleCommands[i]->_arguments[1], "r+");
				//fprintf(f, "\n");
				FILE *temp  = yyin;
				yyin = f;
				clear();
				yyparse();
				yyin = temp;
				fclose(f);
			}
		}else if(strcmp(_simpleCommands[i]->_arguments[0], "setenv") == 0){
			if(_simpleCommands[i]->_numOfArguments < 3){
				perror("Too few arguments");
				exit(-1);
			}else{
				setenv(_simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2],1);
			}
		}else if(strcmp(_simpleCommands[i]->_arguments[0], "unsetenv") == 0){
			if(_simpleCommands[i]->_numOfArguments < 2){
				perror("Too few arguments");
				exit(-1);
			}else{
				unsetenv(_simpleCommands[i]->_arguments[1]);
			}
		}else if(strcmp(_simpleCommands[i]->_arguments[0], "printenv") == 0){
			char ** ptr = environ;
			while(*ptr != NULL){
				printf("%s\n", *ptr);
				ptr++;
			}
			fflush(stdout);
		}else if(strcmp(_simpleCommands[i]->_arguments[0], "cd") == 0){
			if(_simpleCommands[i]->_numOfArguments < 2){
				if(chdir(getenv("HOME")) < 0){
					perror("Error changing directories");
				}
			}else{
				if(chdir(_simpleCommands[i]->_arguments[1]) < 0){
					perror("Error changing directories");
				}
			}
		}else{
			frk = fork();
			if(frk == 0){
				execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
				perror("execvp");
				_exit(1);
				return;
			}else if(frk < 0){
				perror("fork");
				return;
			}
			
		}
	}

	//fflush(stdout);
	dup2(tin, 0);
	dup2(tout, 1);
	dup2(terr, 2);
	close(tin);
	close(tout);
	close(terr);

	if(!_background){
		int stat;
		waitpid(frk, &stat, 0);
		Command::_lastStatus = stat;
	}else{
		_lastPid = frk;
	}
	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec

	// Clear to prepare for next command
	if(_errFile == _outFile){
		_errFile = 0;
	}

	clear();
	
	// Print new prompt
	if(!Command::_justInt){
		prompt();
	}else{
		Command::_justInt = false;
	}
	
	
}

// Shell implementation

void Command::prompt() {
	if(isatty(0)){
		//fflush(stdout);
		printf("myshell>");
		fflush(stdout);
	}
}

int Command::_lastStatus;
bool Command::_justInt = false;
pid_t Command::_lastPid;
Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;
char * Command::_lastArg;
