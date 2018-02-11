/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_Y_TAB_HH_INCLUDED
# define YY_YY_Y_TAB_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 15 "shell.y" /* yacc.c:1909  */

#include <string>
  #include <string.h>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif

#line 53 "y.tab.hh" /* yacc.c:1909  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    WORD = 258,
    STRING = 259,
    SUBSHELL = 260,
    NOTOKEN = 261,
    GREAT = 262,
    NEWLINE = 263,
    PIPE = 264,
    AMPERSAND = 265,
    GREATGREAT = 266,
    LESS = 267,
    GREATAMPERSAND = 268,
    GREATGREATAMPERSAND = 269,
    TWOGREAT = 270,
    END_OF_FILE = 271
  };
#endif
/* Tokens.  */
#define WORD 258
#define STRING 259
#define SUBSHELL 260
#define NOTOKEN 261
#define GREAT 262
#define NEWLINE 263
#define PIPE 264
#define AMPERSAND 265
#define GREATGREAT 266
#define LESS 267
#define GREATAMPERSAND 268
#define GREATGREATAMPERSAND 269
#define TWOGREAT 270
#define END_OF_FILE 271

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 25 "shell.y" /* yacc.c:1909  */

  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;

#line 103 "y.tab.hh" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_HH_INCLUDED  */
