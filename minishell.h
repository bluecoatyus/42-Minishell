#ifndef MINISHELL_H
#define MINISHELL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include "utils/libft.h"

#define DOUBLE_QUOTE "\""
#define SINGLE_QUOTE "'"
#define DOLLAR_SIGN "$"
#define WILD_CARD "*"
#define QUESTION_MARK "?"
#define BRACETS "{}"
#define SLASH "/"

typedef struct	s_ms
{
	int			pid;
	int			error_status;
	char		**ev;
	char		**path;
}				t_ms;

enum tokens{
	TOKEN_PIPE,
	TOKEN_OR,
	TOKEN_AND,
	TOKEN_GREATER,
	TOKEN_SMALLER,
	TOKEN_APPEND,
	TOKEN_HERE_DOC,
	TOKEN_OPEN_PAR,
	TOKEN_CLOSE_PAR,
	TOKEN_STR
}	t_token_type;

typedef struct s_token
{
	enum tokens		type;
	char			*value;
	struct s_token	*prev;
	struct s_token	*next;
}				t_token;

typedef struct s_file
{
	enum tokens		type;
	char			*file_name;
	struct s_file	*next;
}					t_file;

typedef struct s_parsed
{
	int			in_file;
	int			out_file;
	char		*cmd;
	char		**arguments;
	t_token		*paranthesis;
	t_file		*file_list;
	struct s_parsed	*prev;
	struct s_parsed	*next;
}				t_parsed;

//--------------lexer--------------
int			is_metacharacter(char c);
int			token_str_lexer(char *input);
int			add_token(char *input, t_token *command_table, enum tokens type, int len);
void		find_token(char *input, t_token *command_table);
t_token		*tokenizer(char *input);

//--------------expander--------------
int			add_double_quote(char **str, char *val);
int			add_single_quote(char **str, char *val);
int			add_dollar(char **str, char *val);
int			add_char(char **str, char *val);
char		*check_str(char *value);
int			wild_path(char *wild_one, char *expected_one);
void		wildcard(char *path, char **destined_path, int way, t_token **command_table);
void		add_wildcard_to_list(char *path, t_token **command);
t_token		*expander(t_token *command_list);

//--------------expander--------------
int			andor_count(t_token *command_table);
t_parsed	*new_parse_command(int in_file, int out_file);
void		add_andor_list(enum tokens andor, t_parsed *command, t_parsed **andor_table);
void		add_redirection(t_token **command_table, t_parsed **command);
void		skip_paranthesis(t_token **command_table);
t_parsed	*add_parse(t_token **command_table, t_parsed **old_command);
void		add_argument(t_token **command_table, t_parsed **command);
void		add_paranthesis(t_token **command_table, t_parsed **command);
t_parsed	**parse_commands(int in_file, int out_file, t_token *command_table);


#endif
