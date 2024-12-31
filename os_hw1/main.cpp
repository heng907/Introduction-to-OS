/*
Student No.: 111550129
Student Name: 林彥亨
Email: yanheng.cs11@nycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not 
supposed to be posted to a public server, such as a 
public GitHub repository or a public web page.
*/

#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;


void sigchld_handler(int signal)
{
    int state;
    while (waitpid(-1, &state, WNOHANG) > 0);
}

void tokenize(char *input, char **lcmd, char **rcmd, bool &wait_child, int &cmd_status)
{
    bool div = false;
    char *token = strtok(input, " ");
    
    while (token != NULL) 
    {
        //pipe
        if (!strcmp(token, "|")) 
        {
            cmd_status = 1;
            div = true;
        }
        //redirect to  
        else if (!strcmp(token, ">")) 
        {
            cmd_status = 2;
            div = true;
        }
        //redirect from 
        else if (!strcmp(token, "<")) 
        {
            cmd_status = 3;
            div = true;
        } 
        else 
        {
            if (!div)
            {
                *lcmd++ = token;
            } 
            else 
            {
                *rcmd++ = token;
            }
        }
        token = strtok(NULL, " ");
    }

    //background
    if (lcmd > lcmd - 1 && !strcmp(*(lcmd - 1), "&")) 
    {
        wait_child = false;
        *(lcmd - 1) = NULL;
    }
    *lcmd = NULL;

    if (div) 
    {
        *rcmd = NULL;
    }
}

void execmd(pid_t &pid, char *input, bool &wait_child)
{
    int cmd_status = 0;
    char *lcmd[1000], *rcmd[1000];

    tokenize(input, lcmd, rcmd, wait_child, cmd_status);

    pid_t pid1, pid2;
    int pipefd[2];

    switch (cmd_status) {
    //pipe
    case 1: {
        if (pipe(pipefd) == -1) {
            perror("Pipe failed");
            exit(1);
        }

        pid1 = fork();
        if (pid1 == 0) {
            
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(lcmd[0], lcmd);
            perror("execvp failed");
            exit(1);
        }
        else if (pid1 < 0) {
            perror("Fork Failed");
            exit(1);
        }

        pid2 = fork();
        if (pid2 == 0) {
            
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[1]);
            close(pipefd[0]);
            execvp(rcmd[0], rcmd);
            perror("execvp failed");
            exit(1);
        }
        else if (pid2 < 0) {
            perror("Fork Failed");
            exit(1);
        }

        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
        break;
    }
    //redirect to
    case 2: {
        pid1 = fork();
        if (pid1 == 0) {
            int fd = open(rcmd[0], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            execvp(lcmd[0], lcmd);
            perror("execvp failed");
            exit(1);
        }
        else if (pid1 < 0) {
            perror("Fork Failed");
            exit(1);
        }

        waitpid(pid1, NULL, 0);
        break;
    }
    //redirect from
    case 3: {
        pid1 = fork();
        if (pid1 == 0) {
            int fd = open(rcmd[0], O_RDONLY);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            execvp(lcmd[0], lcmd);
            perror("execvp failed");
            exit(1);
        }
        else if (pid1 < 0) {
            perror("Fork Failed");
            exit(1);
        }

        waitpid(pid1, NULL, 0);
        break;
    }
    default:
        pid1 = fork();
        if (pid1 == 0) {
            execvp(lcmd[0], lcmd);
            perror("execvp failed");
            exit(1);
        }
        else if (pid1 < 0) {
            perror("Fork Failed");
            exit(1);
        }
        if (wait_child) 
        {
            waitpid(pid1, NULL, 0);
        } 
        else 
        {
            printf("Process running in background with PID: %d\n", pid1);
        }
        break;
        
    }
}




int main()
{
    pid_t pid;
    char input[1000];
    //zombie check
    signal(SIGCHLD, sigchld_handler);

    cout<<">";
    while (fgets(input, 1000, stdin)) 
    {
        input[strlen(input) - 1] = '\0';
        if (!strcmp(input, "exit") || !strcmp(input, "exit &"))
        {
            break;
        }

        bool wait_child = true;
        execmd(pid, input, wait_child);
        cout<<">";
    }
    return 0;
}
