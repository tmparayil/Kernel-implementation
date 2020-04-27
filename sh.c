
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <glob.h>
#include <stdbool.h>

#define SH_TOK_BUFSIZE 64
#define SH_TOK_DELIM " \t\r\n\a"
#define SH_DELIM ";"


/**
*
* AUTHOR : THOMAS MATHEW PARAYIL
* UCI net ID : tparayil
* STUDENT ID : 93845898
*/


int globErr(const char *epath, int eerrno)
{
    return 0;
}


int getPipeCount(char** args) {
    int i = 0;
    int pipes = 0;

    while(args[i] != NULL) {
        if(strcmp(args[i], "|") == 0) {
            pipes++;
        }
        i++;
    }

    return pipes;
}

/**
 * Takes a command and splits it into tokens separated by space.
 * Collaborated with Amit Somani
 */
char **sh_split_line(char *line)
{
    int bufsize = SH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token, **tokens_backup;
    glob_t paths;
 
    if (!tokens) {
      fprintf(stderr, "sh: allocation error\n");
      exit(EXIT_FAILURE);
    }
 
    token = strtok(line, SH_TOK_DELIM);
    while (token != NULL) {
        if(strcmp(token, "&") != 0) {
            if((strstr(token, "*") != NULL) || (strstr(token, "?") != NULL)) {
                if(glob(token, GLOB_MARK, globErr, &paths) == 0) {
                    for(int i = 0; i < paths.gl_pathc; i++) {
                        tokens[position] = paths.gl_pathv[i];
                        position++;
                        if (position >= bufsize) {
                            bufsize += SH_TOK_BUFSIZE;
                            tokens_backup = tokens;
                            tokens = realloc(tokens, bufsize * sizeof(char *));
                            if (!tokens) {
                                free(tokens_backup);
                                fprintf(stderr, "sh: allocation error\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
          } else {
              tokens[position] = token;
              position++;
          }
    }
         if (position >= bufsize) {
              bufsize += SH_TOK_BUFSIZE;
              tokens_backup = tokens;
              tokens = realloc(tokens, bufsize * sizeof(char *));
              if (!tokens) {
                  free(tokens_backup);
                  fprintf(stderr, "sh: allocation error\n");
                  exit(EXIT_FAILURE);
              }
        }
 
        token = strtok(NULL, SH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

// Para phrased from material provided
char *sh_read_line(void)
{
    char *line = NULL;
    size_t bufsize = 0;  // have getline allocate a buffer for us

    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            fprintf(stderr, "EOF\n");
            exit(EXIT_SUCCESS);
        } else {
        fprintf(stderr, "Value of errno: %d\n", errno);
        exit(EXIT_FAILURE);
        }
    }

    return line;
}

int sh_redirect_execute(char** args)
{
    int in = -1 , out = -1; 
    int i=0;
    int type = -1;

    
    while(args[i] != NULL)
    {
        if(strcmp(args[i],"<") == 0)
        {
            in = i;
        }
        else if(strcmp(args[i],">") == 0)
        {
            out = i;
        }
        i++;
    }

    if(in == -1 && out == -1) type = 0;
    else if(in != -1 && out == -1) type = 1;
    else if(in == -1 && out != -1) type = 2;
    else type = 3;


    switch(type) {
        // No redirection
        case 0: 
        {
            execvp(args[0], args);
            break;
        }

        // Input 
        case 1:
        {
            char* inFile = args[in+1];
            char* setArgs[in+1];

            for(i=0;i<= in -1;i++)
                {
                    setArgs[i] = args[i];
                }
            setArgs[in] = 0;

            close(0);
            open(inFile,O_RDONLY);
            if(execvp(setArgs[0], setArgs) == -1) {
               perror("Error executing");
           }
           break;
        }
        //Output
        case 2:
        {
            char* outFile = args[out+1];

            char* setArgs[out+1];

            for(int i = 0 ; i <= out - 1; i++) {
                setArgs[i] = args[i];
        
            }

            setArgs[out] = 0;
            close(1);
            open(outFile, O_CREAT | O_WRONLY, 0644);
            if(execvp(setArgs[0], setArgs) == -1) {
                perror("Error executing");
            }
            break;
        }
        // Both input & Output
        case 3:
        {
            char* inFile = args[in+1];
            char* outFile = args[out+1];

            char* setArgs[in+1];

            for(int i = 0 ; i <= in - 1; i++) {
                setArgs[i] = args[i];
            }
            setArgs[in] = 0;

            close(0);
            close(1);
            open(inFile, O_RDONLY);
            open(outFile, O_CREAT | O_WRONLY, 0644);

            if(execvp(setArgs[0], setArgs) == -1) {
                perror("Error executing");
            }
            break;

        }
    }

    return 0;
}

/**
 * 
 * RETURNS THE COMMANDS BEFORE THE INDEX OF THE PIPE
 *
 */
char** getCommands(char** args, int index) {

    char** parsedArgs = malloc(SH_TOK_BUFSIZE*sizeof(char*));

    int pipeCount = 0;
    int i = 0 ;
    int j = 0;

    while(args[i] != NULL) {
        if(strcmp(args[i], "|") == 0) {
            pipeCount++;
        }

        if(pipeCount == index) {
            parsedArgs[j] = 0;
            return parsedArgs;
        }
        if((strcmp(args[i], "|") !=0) && (pipeCount == index - 1)) {
            parsedArgs[j] = strdup(args[i]);
            j++;
        }

        i++;
    }

    parsedArgs[j] = 0;
    return parsedArgs;
}

/**
 * Execute Piped commands.
 *
 */

int sh_pipe_execute(char** args, int pipes) {

    int prev_p[2];
    int status;

    int instr = pipes + 1;

    for(int i = 0; i < instr; i++) {
        int curr_p[2];

        // No pipe needed for last command.
        if(i != instr - 1) {
            pipe(curr_p);
        }

        if(fork() == 0) {

            if(i != instr -1) {
                close(curr_p[0]);
                close(1);
                dup(curr_p[1]);
                close(curr_p[1]);
            }
            
            if(i != 0) {
                close(0);
                dup(prev_p[0]);
                close(prev_p[0]);
                close(prev_p[1]);
            }

            char** argList = getCommands(args, i+1);
            sh_redirect_execute(argList);
            free(argList);
        } else {
            if(i != 0) {
                close(prev_p[0]);
                close(prev_p[1]);
            }

            if(i != instr - 1) {
                prev_p[0] = curr_p[0];
                prev_p[1] = curr_p[1];
            }
        }
    }

    if(instr > 1) {
        close(prev_p[0]);
        close(prev_p[1]);
    }

    for(int i = 0; i < pipes + 1; i++) {
        wait(&status);
    }

    return 0;
}

/**
 * Wrapper function for executing piped and non piped commands.
 *
 */
int sh_main_execute(char** args) {
    
    int pipes = getPipeCount(args);

    if(pipes == 0) {
        if(fork() == 0) {
            sh_redirect_execute(args);
        }
        wait(0);
        return 0;
    }
    sh_pipe_execute(args, pipes);    

    return 0;
}

void sh_loop(void)
{
  char *line;
  char **args;
  int status;
  int listCommands = 0;
  char* instr;

  bool amp = false;

  do {
    printf("238P$ ");
    line = sh_read_line();
    char* rest = line;

    while(instr = strtok_r(rest, SH_DELIM, &rest)) {
        char* c = strdup(instr);

        if(strstr(c, "&") != NULL) amp = true;        

        args = sh_split_line(instr);


        if(strcmp(args[0],"exit") == 0)
        {
           printf("GoodBye!\n");
           exit(-1);
        }  

        if(strcmp(args[0], "cd") == 0) {
            if(args[1] == NULL) {
                if(chdir(getenv("HOME")) < 0) {
                    perror("cd error");
                }
            } else {
                if(chdir(args[1]) < 0) {
                    perror("cd : no such directory");
                }
            }
        }


        if(amp) {
            if(fork() == 0) {
                sh_main_execute(args);
            }
        } else {
            sh_main_execute(args);
        }
    }

    free(line);
    free(args);
    } while (1);
}

int main(int argc, char **argv)
{
    sh_loop();
    return EXIT_SUCCESS;
}
