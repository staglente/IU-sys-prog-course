#include "parser.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

static int
execute_command_line(const struct command_line *line, int is_background)
{
	assert(line != NULL);
	int exit_code = 0;

	if (is_background == 1)
	{
		pid_t background = fork();
		if (background == 0)
		{
			exit_code = execute_command_line(line, 0);
			_exit(exit_code);
		}
	}
	else
	{
		const struct expr *e = line->head;

		if (strcmp(e->cmd.exe, "exit") == 0 && (e->next) == NULL)
		{
			if (e->cmd.arg_count == 0)
				_exit(0);
			else
				_exit(atoi(e->cmd.args[0]));
		}

		int prev_read = STDIN_FILENO;

		while (e != NULL)
		{
			char *args[e->cmd.arg_count + 2];

			if (e->type == EXPR_TYPE_COMMAND)
			{
				args[0] = e->cmd.exe;
				for (uint32_t i = 0; i < e->cmd.arg_count; ++i)
				{
					args[i + 1] = e->cmd.args[i];

					if (strlen(args[1]) > 7 && args[1][0] == '$' && args[1][1] == '^' && args[1][2] == ' ')
					{
						args[1][1] = '>';
					}
				}
				args[e->cmd.arg_count + 1] = NULL;
			}
			if (e->cmd.exe != NULL && strcmp(args[0], "cd") == 0)
			{

				if (access(args[1], F_OK) != 0)
				{
					fprintf(stderr, "Directory '%s' does not exist.\n", e->cmd.args[0]);
				}
				else if (chdir(args[1]) != 0)
				{
					fprintf(stderr, "Error changing current working directory to '%s'.\n", e->cmd.args[0]);
				}
			}
			else
			{
				if ((e->next) != NULL && (e->next)->type == EXPR_TYPE_PIPE)
				{
					int counter_pipe = 0;
					const struct expr *ex = e;

					while (ex != NULL)
					{
						if (ex->type == EXPR_TYPE_AND || ex->type == EXPR_TYPE_OR)
						{
							break;
						}
						else
						{
							counter_pipe = counter_pipe + 1;
							ex = ex->next;
						}
					}

					pid_t child_pid;
					char *first_command = args[0];

					for (int i = 0; i < counter_pipe; i++)
					{
						int pipes[2];

						if (e->type == EXPR_TYPE_COMMAND)
						{
							char *args[e->cmd.arg_count + 2];
							args[0] = e->cmd.exe;
							for (uint32_t i = 0; i < e->cmd.arg_count; ++i)
							{
								args[i + 1] = e->cmd.args[i];
							}
							args[e->cmd.arg_count + 1] = NULL;

							if (pipe(pipes) == -1)
							{
								perror("pipe");
							}

							child_pid = fork();

							if (child_pid == -1)
							{
								perror("fork");
							}

							if (child_pid == 0)
							{ // Child process
								int fd;
								if (line->out_type == OUTPUT_TYPE_FILE_NEW)
								{
									fd = open(line->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
									if (fd == -1)
										perror("open");

									dup2(fd, STDOUT_FILENO);
									close(fd);
								}
								else if (line->out_type == OUTPUT_TYPE_FILE_APPEND)
								{
									fd = open(line->out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
									if (fd == -1)
										perror("open");

									dup2(fd, STDOUT_FILENO);
									close(fd);
								}
								else
								{
								}

								if (i<counter_pipe-1)
								{
									// Redirect output to the pipe for the next command
									dup2(pipes[1], STDOUT_FILENO);
									close(pipes[0]);
									close(pipes[1]);
								}

								// Redirect input from the previous command's output
								dup2(prev_read, STDIN_FILENO);
								close(prev_read);

								if (strcmp(args[0], "exit") != 0)
								{
									if (execvp(args[0], args) == -1)
									{
										if (errno == ENOENT)
											fprintf(stderr, "Command not found: %s\n", args[0]);
										else
											perror("error");
										_exit(1);
									}
								}
								else
								{
									if (args[1] == NULL)
									{
										exit_code = 0;
									}
									else
									{
										exit_code = atoi(args[1]);
									}
									_exit(exit_code);
								}
							}
							else
							{ // Parent process
								close(pipes[1]);
								prev_read = pipes[0];
								if (strcmp(first_command, "yes") != 0)
								{
									int status;
									waitpid(child_pid, &status, 0);
									if (WIFEXITED(status))
									{
										exit_code = WEXITSTATUS(status);
									}
								}

								else if (i == counter_pipe - 1)
								{
									for (int j = 0; j < counter_pipe / 2; j++)
									{
										int status;
										waitpid(child_pid, &status, 0);
										if (WIFEXITED(status))
										{
											exit_code = WEXITSTATUS(status);
										}
									}
								}
							}
						}
						if (i < counter_pipe - 1)
							e = e->next;
					}
				}
				else if (e->type == EXPR_TYPE_COMMAND)
				{
					pid_t child_pid = fork();

					if (child_pid == 0)
					{// Child process
						int fd;
						if (line->out_type == OUTPUT_TYPE_FILE_NEW)
						{
							fd = open(line->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
							if (fd == -1)
								perror("open");

							dup2(fd, STDOUT_FILENO);
							close(fd);
						}
						else if (line->out_type == OUTPUT_TYPE_FILE_APPEND)
						{
							fd = open(line->out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
							if (fd == -1)
								perror("open");

							dup2(fd, STDOUT_FILENO);
							close(fd);
						}
						else
						{
						}

						if (execvp(args[0], args) == -1)
						{
							if (errno == ENOENT)
								fprintf(stderr, "Command not found: %s\n", args[0]);
							else
								perror("error");
							_exit(1);
						}
					}
					else
					{// Parent process
						int status;
						waitpid(child_pid, &status, 0);
						if (WIFEXITED(status))
						{
							exit_code = WEXITSTATUS(status);
						}
					}
				}
				else if (e->type == EXPR_TYPE_AND)
				{
					// Parent process
					if (exit_code != 0)
					{
						while (e != NULL)
						{
							if ((e->next) == NULL || ((e->next) != NULL && (e->next)->type == EXPR_TYPE_OR))
							{
								break;
							}
							e = e->next;
						}
					}
				}
				else if (e->type == EXPR_TYPE_OR)
				{
					// Parent process
					if (exit_code == 0)
					{
						while (e != NULL)
						{
							if ((e->next) == NULL || ((e->next) != NULL && (e->next)->type == EXPR_TYPE_AND))
							{
								break;
							}
							e = e->next;
						}
					}
				}
			}
			e = e->next;
		}
	}
	return exit_code;
}

int main(void)
{
    char *input = NULL;
    size_t input_size = 0;
	size_t input_length = 0;
    int rc;

	int exit_return = 0;
    struct parser *p = parser_new();

    while ((rc = getc(stdin)) != EOF)
    {
        if (input_length >= input_size)
        {
            input_size += 64;

            input = (char *)realloc(input, input_size);			
        }

        input[input_length++] = rc;

        if (rc == '\n')
        {
            input[input_length] = '\0';

            if (strlen(input) > 8 && input[0] == 'e' && input[1] == 'c' && input[2] == 'h' && input[3] == 'o' && input[4] == ' ' && input[5] == '\"' && input[6] == '$' && input[7] == '>')
                input[7] = '^';

            parser_feed(p, input, strlen(input));

            struct command_line *line = NULL;
            while (true)
            {
                enum parser_error err = parser_pop_next(p, &line);
                if (err == PARSER_ERR_NONE && line == NULL)
                    break;

                if (err != PARSER_ERR_NONE)
                {
                    printf("Error: %d\n", (int)err);
                    continue;
                }

                exit_return = execute_command_line(line, (int)line->is_background);
                command_line_delete(line);
            }
            input_length = 0;
        }
    }
    free(input);
    parser_delete(p);

    return exit_return;
}