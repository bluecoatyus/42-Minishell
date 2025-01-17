#include "../minishell.h"

void close_fd(t_parsed *command)
{
    if (command->in_file && command->in_file != STDIN_FILENO)
        close(command->in_file);
    if (command->out_file && command->out_file != STDOUT_FILENO)
        close(command->out_file);
}

void close_fd_parantheses(t_parsed *command)
{
    int i;
    t_parsed *tmp_command;
    t_parsed **tmp_andor_table;

    if (command->paranthesis)
    {
        tmp_andor_table = command->parantheses_andor;
        i = -1;
        while (tmp_andor_table[++i])
        {
            tmp_command = tmp_andor_table[i];
            while (tmp_command)
            {
                close_fd(tmp_command);
                if (tmp_command->paranthesis)
                    close_fd(command);
                tmp_command = tmp_command->next;
            }
        }
    }
}

int here_doc_fd(char *limiter)
{
    char *input;
    char *final_line;
    char *tmp;
    int fd[2];

    input = readline(">");
    final_line = (char *)ft_calloc(2, sizeof(char));
    while (ft_strcmp(limiter, input))
    {
        tmp = input;
        input = ft_strjoin(input, "\n");
        free(tmp);
        tmp = final_line;
        final_line = ft_strjoin(final_line, input);
        free(tmp);
        free(input);
        input = readline(">");
    }
    free(input);
    free(limiter);
    if (pipe(fd) == -1)
        print_error(PIPE_ERR, NULL);
    write(fd[WRITE_END], final_line, ft_strlen(final_line));
    free(final_line);
    close(fd[WRITE_END]);
    return (fd[READ_END]);
}

int read_file_fd(char *file_name, int type)
{
    if (type == TOKEN_HERE_DOC)
        return (here_doc_fd(file_name));
    if (access(file_name, F_OK) == -1)
        print_error(FILE_NOT_FOUND, file_name);
    if (access(file_name, R_OK) == -1)
        print_error(PERM_DENIED, file_name);
    return (open(file_name, O_RDONLY, 0666));
}

int write_file_fd(char *file_name, int type)
{
    if (type == TOKEN_GREATER)
        return (open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0666));
    else if (type == TOKEN_APPEND)
        return (open(file_name, O_CREAT | O_WRONLY | O_APPEND, 0666));
    return -1;
}

void apply_redirection(t_parsed **command)
{
    t_file *file_list;

    file_list = (*command)->file_list;
    if (!file_list)
        return ;
    while (file_list)
    {
        if (file_list->type == TOKEN_SMALLER)
            (*command)->in_file = read_file_fd(file_list->file_name, file_list->type);
        else if (file_list->type == TOKEN_HERE_DOC)
            (*command)->in_file = read_file_fd(file_list->file_name, file_list->type);
        else if (file_list->type == TOKEN_GREATER)
            (*command)->out_file = write_file_fd(file_list->file_name, file_list->type);
        else if (file_list->type == TOKEN_APPEND)
            (*command)->out_file = write_file_fd(file_list->file_name, file_list->type);
        file_list = file_list->next;
    }
}

void child_organizer(t_parsed *command)
{
    pid_t pid;

    if ((pid = fork()) < 0)
        print_error(FORK_ERR, NULL);
    if (!pid)
    {
	    g_ms.parent_pid = getpid();
        dup2(command->in_file, g_ms.in_file);
        dup2(command->out_file, g_ms.out_file);
        g_ms.in_file = command->in_file;
        g_ms.out_file = command->out_file;
        organizer(command->parantheses_andor);
        close_fd(command);
        exit(0);
    }
    waitpid(pid, &errno, 0);
    close_fd(command);
    close_fd_parantheses(command);
}

void command_executor(t_parsed *command)
{
    pid_t pid;
	int	in;
    int	out;

    expander(&command);
    if (is_builtin(command->cmd))
    {
        errno = 0;
	    in = dup(g_ms.in_file);
    	out = dup(g_ms.out_file);
        dup2(command->in_file, g_ms.in_file);
        dup2(command->out_file, g_ms.out_file);
        run_builtin(command->arguments);
        close_fd(command);
        dup2(in, g_ms.in_file);
        dup2(out, g_ms.out_file);
    }
    else
    {
        pid = fork();
        g_ms.child_pids[g_ms.child_pids_count++] = pid;
        if ((pid) < 0)
            print_error(FORK_ERR, NULL);
        if (!pid)
        {
            errno = 0;
            dup2(command->in_file, g_ms.in_file);
            dup2(command->out_file, g_ms.out_file);
            close_fd(command);
            execve(get_path(command->cmd), command->arguments, g_ms.ev);
            print_error(CMD_NOT_FOUND, command->cmd);
            exit (errno);
        }
        close_fd(command);
    }
}

void create_pipe(t_parsed **command)
{
    int fd[2];

    if (pipe(fd) == -1)
        print_error(PIPE_ERR, NULL);
    if ((*command)->out_file != g_ms.out_file && (*command)->out_file != STDOUT_FILENO)
        close((*command)->out_file);
    (*command)->out_file = fd[WRITE_END];
    if ((*command)->next->in_file != g_ms.out_file && (*command)->next->in_file != STDIN_FILENO)
        close((*command)->next->in_file);
    (*command)->next->in_file = fd[READ_END];
}

void create_redirections(t_parsed **andor_table)
{
    int i;
    t_parsed *tmp_command;

    i = -1;
    while (andor_table[++i])
    {
        tmp_command = andor_table[i];
        while (tmp_command)
        {
            if (tmp_command->next)
                create_pipe(&tmp_command);
            apply_redirection(&tmp_command);
            if (tmp_command->paranthesis)
            {
                tmp_command->parantheses_andor = parse_commands(tmp_command->in_file, tmp_command->out_file, tmp_command->paranthesis);
                create_redirections(tmp_command->parantheses_andor);
            }
            tmp_command = tmp_command->next;
        }
    }
}

void organizer(t_parsed **andor_table)
{
    int i;
    t_parsed *tmp_command;

    i = -1;
    while (andor_table[++i])
    {
        tmp_command = andor_table[i];
        if (tmp_command->exec == 3
            || (tmp_command->exec == TOKEN_AND && errno == 0)
            || (tmp_command->exec == TOKEN_OR && errno != 0))
            while (tmp_command)
            {
                if (tmp_command->paranthesis)
                    child_organizer(tmp_command);
                else
                    command_executor(tmp_command);
                tmp_command = tmp_command->next;
            }
    }
}

void executor(t_parsed **andor_table)
{
    create_redirections(andor_table);
    organizer(andor_table);
    int i = 0;
    int status;
    while (i < g_ms.child_pids_count)
        waitpid(g_ms.child_pids[i++], &status, 0);
    ft_bzero(g_ms.child_pids, sizeof(int) * g_ms.child_pids_count);
    /* 1. problem başlangıçta kaç tane process açacağını 
    bilmediğimden dolayı 100lük limit koydum(inite bak) ama sen 10000de koyabilirsin
    VLA sayılmıyor, 2. problem burada ft_bzero atıyorum ama*/
    g_ms.child_pids_count = 0;
}
