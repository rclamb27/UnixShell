/*
Ryan Lamb
3/22/2021
Unix-Shell Project
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define READ 0 
#define WRITE 1

int find_pipe(char** cargs);

char** tokparse(char* input, char* cargs[])
{
    const char d[2] = {' ', '\t'};
    char* symb;
    int args_num = 0;

    symb = strtok(input, d);
    cargs[0] = symb;

    char** read = malloc(2 * sizeof(char*));
    for(int x = 0; x < 2; ++x)
        read[x] = malloc(BUFSIZ * sizeof(char));
    read[0] = "";
    read[1] = "";

    while(symb != NULL)
    {
        symb = strtok(NULL, d);
        if(symb == NULL) break;
        if(!strncmp(symb, ">", 1))                  
        {
            symb = strtok(NULL, d);
            read[0] = "o";
            read[1] = symb;
            return read;
        } 
        else if(!strncmp(symb, "<", 1))
        {
            symb = strtok(NULL, d);
            read[0] = "i";
            read[1] = symb;
            return read;
        }
        else if(!strncmp(symb, "|", 1))
        {
            read[0] = "p";        
        }
        cargs[++args_num] = symb;
    }
    return read;
}

int main(int argc, const char* argv[])
{
    char input [BUFSIZ];
    char mru   [BUFSIZ]; 
    int pfd[2];

    
    memset(input, 0, BUFSIZ * sizeof(char));
    memset(mru,   0, BUFSIZ * sizeof(char));

    while(1)
    {
        printf("osh> ");  
        fflush(stdout);   
        
        fgets(input, BUFSIZ, stdin);
        input[strlen(input) - 1] = '\0'; 
        
        if(strncmp(input, "exit", 4) == 0)
            return 0;
        if(strncmp(input, "!!", 2)) 
            strcpy(mru, input);      
       
        bool dowait = true;
        char* wait_offset = strstr(input, "&");
        if(wait_offset != NULL){*wait_offset = ' '; dowait=false;}

        pid_t pid = fork();
        if(pid < 0)
        {   
            fprintf(stderr, "fork failed...\n");
            return -1; 
        }
        if(pid != 0)
        {  // parent
            if(dowait)
            {
                wait(NULL);
                wait(NULL);
            }
            
        }
        else
        {          
            char* cargs[BUFSIZ];    
            memset(cargs, 0, BUFSIZ * sizeof(char));

            int hist = 0;
            // use of '!!'
            if(!strncmp(input, "!!", 2)) hist = 1;
           
            char** redirect = tokparse( (hist ? mru : input), cargs);
            
            if(hist && mru[0] == '\0')
            {
                printf("No recently used command.\n");
                exit(0);
            } 
            
            if(!strncmp(redirect[0], "o", 1))
            {
                printf("output saved to ./%s\n", redirect[1]);
                int fd = open(redirect[1], O_TRUNC | O_CREAT | O_RDWR);
                dup2(fd, STDOUT_FILENO); 
            }
            else if(!strncmp(redirect[0], "i", 1))
            {
                printf("reading from file: ./%s\n", redirect[1]);
                int fd = open(redirect[1], O_RDONLY);
                memset(input, 0, BUFSIZ * sizeof(char));
                read(fd, input,  BUFSIZ * sizeof(char));
                memset(cargs, 0, BUFSIZ * sizeof(char));
                input[strlen(input) - 1]  = '\0';
                tokparse(input, cargs);
            }
            else if(!strncmp(redirect[0], "p", 1))
            {
                pid_t pidc;
                int pipe_rhs_offset = find_pipe(cargs);
                cargs[pipe_rhs_offset] = "\0";
                int e = pipe(pfd);
                if(e < 0)
                {
                    fprintf(stderr, "pipe creation failed...\n");
                    return 1;
                }

                char* lhs[BUFSIZ], *rhs[BUFSIZ];
                
                memset(lhs, 0, BUFSIZ*sizeof(char));
                memset(rhs, 0, BUFSIZ*sizeof(char));

                for(int x = 0; x < pipe_rhs_offset; ++x)
                {
                    lhs[x] = cargs[x];
                }
                for(int x = 0; x < BUFSIZ; ++x)
                {
                    int idx = x + pipe_rhs_offset + 1;
                    if(cargs[idx] == 0) break;
                    rhs[x] = cargs[idx];
                }
                
                pidc = fork();//child reat
                if(pidc < 0){
                    fprintf(stderr, "fork failed...\n");
                    return 1;
                }
                if(pidc != 0)// parent process 
                {
                    dup2(pfd[WRITE], STDOUT_FILENO);
                    close(pfd[WRITE]);
                    execvp(lhs[0], lhs);
                    exit(0); 
                }
                else // child process
                {
                    dup2(pfd[READ], STDIN_FILENO);
                    close(pfd[READ]);
                    execvp(rhs[0], rhs);
                    exit(0); 
                }
                wait(NULL);
            }
            execvp(cargs[0], cargs);
            exit(0);  
        }
    }

    return 0;
}


int find_pipe(char** cargs){
    int i = 0;
    while(cargs[i] != '\0')
    {
        if(!strncmp(cargs[i], "|", 1))
        {
            return i;
        }
        ++i;
    }
    return -1;
}
