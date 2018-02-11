#ifndef command_h
#define command_h

#include "simpleCommand.hh"

// Command Data Structure

struct Command {
  int _numOfAvailableSimpleCommands;
  int _numOfSimpleCommands;
  SimpleCommand ** _simpleCommands;
  char * _outFile;
  char * _inFile;
  char * _errFile;
  int _background;
  int _append;

  void prompt();
  void print();
  void execute();
  void clear();

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  static Command _currentCommand;
  static char *  _lastArg;
  static int _lastPid;
  static int _lastStatus;
  static SimpleCommand *_currentSimpleCommand;
  static bool _justInt;
};

#endif
