#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>

#define SUCCESS 0
#define FAILURE 1
#define WR_END 1
#define RD_END 0

//UTILS:
size_t	ft_strlen(char *str)
{
	size_t	len = 0;
	while (*str++)
		len++;
	return (len);
}

void	ft_putstr_fd(int fd, char *str)
{
	write(fd, str, ft_strlen(str));
}

//ERROR:
void	print_error(char *msg1, char *msg2)
{
	ft_putstr_fd(STDERR_FILENO, "error: ");
	if (msg1)
		ft_putstr_fd(STDERR_FILENO, msg1);
	if (msg2)
		ft_putstr_fd(STDERR_FILENO, msg2);
	ft_putstr_fd(STDERR_FILENO, "\n");
}

void	error_exit(void)
{
	print_error("fatal", NULL);
	exit(FAILURE);
}

//CD:
int	execute_cd(char **av, int av_size)
{
	if (av[av_size + 1] == NULL)
		av_size++;
	if (av_size != 2)
		return (print_error("cd: bad arguments", NULL), FAILURE);
	if (chdir(av[1]) == -1)
		return (print_error("cd: cannot change directory to ", av[1]), FAILURE);
	return (SUCCESS);	
}

//CMD:
int	execute_cmd(char **av, int av_size, char **env)
{
	int	exit_code = SUCCESS;
	bool	piping = false;
	int		fds[2];
	pid_t	cpid = 0;
	
	if (av[av_size][0] == '|')
		piping = true;
	if (piping && pipe(fds) == -1)
		error_exit();
	cpid = fork();
	if (cpid == -1)
		error_exit();
	if (cpid == 0)
	{
		if (piping)
		{
			if (dup2(fds[WR_END], STDOUT_FILENO) == -1)
			{
				close(fds[WR_END]);
				close(fds[RD_END]);	
				error_exit();
			}
			close(fds[WR_END]);
		}
		if (av[av_size][0] == '|' || av[av_size][0] == ';')
			av[av_size] = NULL;
		else if (av[av_size + 1] == NULL)
			av_size++;
		if (execve(av[0], av, env) == -1)
			return (print_error("cannot execute ", av[0]), FAILURE);	
	}
	if (piping)
	{
		close(fds[WR_END]);
		if (dup2(fds[RD_END], STDIN_FILENO) == -1)
		{
			close(fds[RD_END]);
			error_exit();
		}
		close(fds[RD_END]);
	}
	waitpid(cpid, &exit_code, WUNTRACED);
	return (exit_code);
}

int	main(int ac, char **av, char **env)
{
	int	exit_code = SUCCESS;
	int	av_size = 0;
	
	if (ac < 2)
		return (exit_code);
	av++;
	while (av[av_size])
	{
		if (av[av_size][0] == '|' || av[av_size][0] == ';' || av[av_size + 1] == NULL)
		{
			if (strcmp(av[0], "cd") == 0)
				exit_code = execute_cd(av, av_size);
			else if (av_size == 0 && (av[av_size][0] == '|' || av[av_size][0] == ';'))
			{
				av++;
				continue;
			}
			else
				exit_code = execute_cmd(av, av_size, env);
			av += av_size + 1;
			av_size = 0;
			continue;
		}
		av_size++;
	}
	return (SUCCESS);
}
