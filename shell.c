#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/stat.h>

int shell_cd(char **);
int shell_help(char **);
int shell_exit(char **);
int shell_pwd(char **);
int shell_echo(char **);
int shell_mkdir(char **);
int shell_rmdir(char **);
int shell_history(char **);
int argc;

#define HISTORY_SIZE 1024
#define MAX_SIZE 128
char *history[HISTORY_SIZE];
int history_pos=0;


char *builtins[]={
    "cd",
    "help",
    "exit",
    "pwd",
    "echo",
    "mkdir",
    "rmdir",
    "history"
};

int (*builtin_func[])(char **)={
    &shell_cd,
    &shell_help,
    &shell_exit,
    &shell_pwd,
    &shell_echo,
    &shell_mkdir,
    &shell_rmdir,
    &shell_history  
};

int size_of_builtins(){
    return sizeof(builtins)/sizeof(char*);
}

int shell_cd(char **args){
    if(args[1]==NULL)fprintf(stderr,"Error\n");
    else if(chdir(args[1]))perror("Error\n");
    return 1;
}

int shell_help(char **args){
    printf("Help :\n");
    printf("The following commands are built in:\n");

    for(int i=0;i<size_of_builtins();i++){
        printf("%d - %s\n",i+1,builtins[i]);
    }
    return 1;
}

int shell_exit(char **args){
    return 0;
}

int shell_pwd(char **args){
    char* directory=getcwd(NULL,0);
    if(!directory){
        fprintf(stderr,"Error displaying current directory\n");
        return 0;
    }
    printf("%s\n",directory);
    free(directory);
    return 1;
}

int shell_echo(char **args){
    for(int i=1;;i++){
        if(args[i]==NULL)break;
        printf("%s ",args[i]);
    }
    printf("\n");
    return 1;
}

int shell_mkdir(char **args){
    if(mkdir(args[1],0777)==-1){
        fprintf(stderr,"Error creating directory\n");
        return 0;
    }
    return 1;
}

int shell_rmdir(char **args){
    int removed=!(rmdir(args[1]));
    if(!removed)fprintf(stderr,"Failed in removing directory\n");
}


int append_history(char *line){
    if(history_pos==HISTORY_SIZE){
        fprintf(stderr,"Exceeded storage limit!\n");
        exit(EXIT_FAILURE);
        return 0;
    }
    char *newLine=malloc(sizeof(char*)*MAX_SIZE);
    strcpy(newLine,line);
    history[history_pos]=newLine;
    history_pos++;
    return 1;
}

int shell_history(char **args){
    for(int i=0;i<history_pos;i++){
        printf("%d) %s \n",i+1,history[i]);
    }
    printf("\n");
    return 1;
}


int shell_launch(char **args){
    pid_t pid;
    int status;
    pid=fork();
    if(!pid){//in child process
        if(execvp(args[0],args)==-1)perror("Error\n");
        exit(EXIT_FAILURE);
    }else if(pid<0)perror("Error\n");//couldn't create process
    else{
        pid_t wpid;
        do{//wait until the process gets a turn to execute
            wpid=waitpid(pid,&status,WUNTRACED);
        }while(!WIFEXITED(status)&&!WIFSIGNALED(status));
    }
    return 1;
}

int shell_execute(char **args){
    if(args[0]==NULL)return 1;
    for(int i=0;i<size_of_builtins();i++){
        if(strcmp(args[0],builtins[i])==0)return (*builtin_func[i])(args);
    }
    return shell_launch(args);
}

#define BUFFER_SIZE 1024

char* shell_read_line(){
    int buffer_size=BUFFER_SIZE;
    int pos=0;
    char *buffer=malloc(sizeof(char)*buffer_size);

    if(!buffer){
        fprintf(stderr,"Couldn't allocate buffer\n");
        exit(EXIT_FAILURE);
    }

    int c;
    while(1){
        c=getchar();
        if(c==EOF||c=='\n'){//terminate buffer with a null
            buffer[pos]='\0';
            return buffer;
        }else buffer[pos]=c;
        pos++;
        if(pos>=buffer_size){
            //reallocate
            buffer_size+=BUFFER_SIZE;
            buffer=realloc(buffer,buffer_size);
            if(!buffer){//failure in reallocation
                fprintf(stderr,"Couldn't allocate buffer\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define TOKEN_BUFFER_SIZE 64
#define DELIMITER " \t\r\n\a"

char **shell_split_line(char *line){
    //split the input command into tokens
    //based on delimiter 
    int buffer_size=TOKEN_BUFFER_SIZE;
    int pos=0;
    char **tokens=malloc(buffer_size*sizeof(char*));
    char *token;

    if(!tokens){
        fprintf(stderr,"Couldn't allocate\n");
        exit(EXIT_FAILURE);
    }

    token=strtok(line,DELIMITER);
    while(token){
        tokens[pos]=token;
        pos++;
        argc++;
        if(pos>=buffer_size){
            buffer_size+=BUFFER_SIZE;
            tokens=realloc(tokens,buffer_size*sizeof(char*));
            if(!tokens){
                fprintf(stderr,"Couldn't allocate\n");
                exit(EXIT_FAILURE);
            }
        }
        token=strtok(NULL,DELIMITER);
    }
    tokens[pos]=NULL;
    return tokens;
}

void shell_loop(){
    char *line;
    char **args;
    int status;

    do{
        printf("> ");
        line=shell_read_line();
        append_history(line);
        args=shell_split_line(line);
        status=shell_execute(args);

        free(line);
        free(args);
    }while(status);
}

int main(int argc, char **argv[]){
    shell_loop();
    return 0;   
}