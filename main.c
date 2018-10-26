#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_LEN 100
#define MAX_ARGS 25
#define DEL " "

void readLine(char line[]);
void removeNewLine(char line[]);
void shellLoop();
int parseLine(char line[], char* command[]);
int setBckGrndFlag(char* command[], int argsNum);
void execute (char* command[], int bckGrndFlag);
void signalHandler (int signal);
void logfile(int processPID);

FILE *fptr;

int main()
{
    //start shell loop here
    shellLoop();

    //shell exit
    exit(EXIT_SUCCESS);

    return 0;
}

void shellLoop() {
    //variable to read user input
    char line[MAX_LEN];
    //status to end loop
    int status = 1;
    //command and its args
    char* command[MAX_ARGS];
    //background flag for child process execution
    int bckGrndFlag;
    int argsNum;


    fptr = fopen("logfile.txt","w");
    do {
        printf("Shell >> ");
        readLine(line);
        //check if user input an empty line
        if (strcmp(line, "") == 0) {
            continue;
        }
        //check if command entered is exit
        if (strcmp(line, "exit") == 0) {
            status = 0;
            continue;
        }
	//parse line read and set args+command
        argsNum = parseLine(line, command);

        if (argsNum == 0) {
            continue;
        }
	//set background flag
        bckGrndFlag = setBckGrndFlag(command, argsNum);

        //start command execution
        execute(command, bckGrndFlag);

	//reset flags
        bckGrndFlag = 0;
        argsNum = 0;

    } while(status);


}

//reads line from user
void readLine(char line[]) {
    //read line of 100 chars from stdin
    fgets(line, 100, stdin);
    //remove new line added due to fgets method
    removeNewLine(line);
}
//remove the new line at the end of user input
void removeNewLine(char line[]) {
    int i = 0;
    while (line[i] != '\n') {
        i++;
    }
    line[i] = '\0';
}
//split line into command and args
int parseLine(char line[], char* command[]) {
    //use single space as delimiter
    command[0] = strtok(line, DEL);

    int i = 0;
    while(command[i] != NULL) {
        command[++i] = strtok(NULL, DEL);
    }
    return i;
}

//check for background action and remove & from args list
int setBckGrndFlag(char* command[], int argsNum) {
    if (argsNum > 1 && strcmp(command[argsNum - 1], "&") == 0) {
        command[argsNum - 1] = NULL;
        return 1;
    }
    return 0;
}

void execute (char* command[], int bckGrndFlag) {
    pid_t childPID;
    int status;
    //signal for when the child process ends
    signal(SIGCHLD, signalHandler);

    //fork parent process
    childPID = fork();

    //execuute child process, forking return = 0
    if (childPID == 0) {
        //used for synchronization
        setpgrp();
        //execute command
        execvp(command[0], command);

        //set error if execution failed
        perror("Process execution failed");
        exit(EXIT_FAILURE);

    } else if (childPID < 0) {
        //forking of parent failed
        perror("Forking failed");
        exit(EXIT_FAILURE);
    } else {
	//used to prevent racing
        setpgid(childPID, childPID);
        if (bckGrndFlag == 0) {
	    //wait until child process is executed and when terminated print in file
            pid_t wpid;
            while((wpid = waitpid(childPID, &status, WNOHANG)) == 0);
            logFile(wpid);
        } else {
            printf("%d\n", childPID);
        }
    }

}

//end child process handler
void signalHandler (int signal) {

  if (signal==SIGCHLD) {
      //wait for child to end and print the pid of ended child
        while (waitpid(-1, NULL, WNOHANG) > 0 ) {
            //Process is terminated
            logFile((int)SIGCHLD);
        }
    }
}
//enter data in log file
void logFile(int processPID) {
    fptr = fopen("logfile.txt","a");
    fprintf(fptr, "Child process ended with pid : %d\n", processPID);
    fclose(fptr);

}
