// Eden Ahady 318948106

#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include <sys/wait.h>
#include "string.h"
#define MAX_LINE_LENGTH 150
#define ERROR_FILE "errors.txt"
#define RESULTS_FILE "results.csv"
#define USER_OUTPUT_FILE "userOutput.txt"
//read all the chars from the file given with fd, until it reaches '\n'
void errorHandler(int error, const char *whatToPrint){
    if(error == -1 && strcmp(whatToPrint, "read") == 0){
        if (write(1, "Error in: read", strlen("Error in: read")) == -1) {}
        exit(-1);
    }
    if(error == -1 && strcmp(whatToPrint, "open") == 0){
        if (write(1, "Error in: open", strlen("Error in: open")) == -1) {}
        exit(-1);
    }
    if(error == 0 && strcmp(whatToPrint, "directory") == 0){
        if (write(1, "Not a valid directory\n", strlen("Not a valid directory\n")) == -1) {}
        exit(-1);
    }
    if(error == -1 && strcmp(whatToPrint, "input") == 0){
        if (write(1, "Input file not exist\n", strlen("Input file not exist\n")) == -1) {}
        exit(-1);
    }
    if(error == -1 && strcmp(whatToPrint, "output") == 0){
        if (write(1, "Output file not exist\n", strlen("Output file not exist\n")) == -1) {}
        exit(-1);
    }
}

void readOneLine(int fd, char *buffer) {
    int i = 0;
    char commandName1[1024] = "read";
    while (1) {
        int bytesRead = read(fd, &buffer[i], 1);
        errorHandler(bytesRead, commandName1);
        if (bytesRead == 0 || buffer[i] == '\n') {
            buffer[i] = '\0';
            break;
        }
        ++i;
    }
}


int main(int argc, char *argv[]) {
    //Opening the configuration file and separating its 3 lines
    int config = open(argv[1], O_RDONLY);
    system("ls");
    system("pwd");
    char commandName2[1024] = "open";
    errorHandler(config, commandName2);
    char subFolder[MAX_LINE_LENGTH], inputPath[MAX_LINE_LENGTH], output[MAX_LINE_LENGTH];
    readOneLine(config, subFolder);
    readOneLine(config, inputPath);
    readOneLine(config, output);
    close(config);

    //Opening files
    struct stat firstArgStat;
    if (stat(subFolder, &firstArgStat) == -1) {
        perror("stat");
        exit(-1);
    }
    if (!S_ISDIR(firstArgStat.st_mode)) {
        char commandName3[1024] = "directory";
        errorHandler(0, commandName3);
    }

    int input = open(inputPath, O_RDONLY);
    char commandName4[1024] = "input";
    errorHandler(input, commandName4);

    int correctOutput = open(output, O_RDONLY);
    char commandName5[1024] = "output";
    errorHandler(correctOutput, commandName5);

    int fd;
    fd = open(ERROR_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    errorHandler(fd, commandName2);

    int result;
    result = open(RESULTS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    errorHandler(result, commandName2);

    //open all the folders
    DIR *studentsDir;
    if ((studentsDir = opendir(subFolder)) == NULL) {
        if (write(1, "Error in: opendir", strlen("Error in: opendir")) == -1) {}
        exit(-1);
    }

    struct dirent *folder;
    char studentFolderPath[MAX_LINE_LENGTH];
    while ((folder = readdir(studentsDir)) != NULL) {
        //Creating full path to the student's folder

        size_t subFolderLen = strlen(subFolder);
        size_t folderNameLen = strlen(folder->d_name);
        if (subFolderLen + 1 + folderNameLen >= MAX_LINE_LENGTH) {
            // handle error: path is too long
            continue;
        }
        strcpy(studentFolderPath, subFolder);
        studentFolderPath[subFolderLen] = '/';
        strcpy(studentFolderPath + subFolderLen + 1, folder->d_name);
        struct stat folderStat;
        //Checking if the file we're iterating through is indeed a folder
        if (stat(studentFolderPath, &folderStat) == -1) {
            if (write(1, "Error in: stat", strlen("Error in: stat")) == -1) {}
            continue;
        }
        if (!S_ISDIR(folderStat.st_mode) || strcmp(folder->d_name, "..") == 0 || strcmp(folder->d_name, ".") == 0)
            continue;
        int userFD;

    // Open student folder and iterate through files
    DIR* dir = opendir(studentFolderPath);
    if (dir == NULL) {
        if (write(1, "Error in: opendir", strlen("Error in: opendir")) == -1) {}
        continue;
    }
    struct dirent* entry;
    char cFilePath[MAX_LINE_LENGTH];
    int fileFound = 0;
    while ((entry = readdir(dir)) != NULL) {
        // Check if file is a C file
        int len = strlen(entry->d_name);
        if (len >= 2 && entry->d_name[len - 2] == '.' && entry->d_name[len - 1] == 'c') {
            // Create full path to file
            strcpy(cFilePath, studentFolderPath);
            strcat(cFilePath, "/");
            strcat(cFilePath, entry->d_name);
            struct stat fileStat;
            if (stat(cFilePath, &fileStat) == -1) {
                write(STDOUT_FILENO, "stat error\n", 12);
                continue;
            }
            if (!S_ISREG(fileStat.st_mode)) {
                continue;
            }

            // If it is a C file, compile it
            fileFound = 1;
            int gccStatus, gccRet;
            pid_t pid = fork();
            if (pid == -1) {
                if (write(1, "Error in: fork", strlen("Error in: fork")) == -1) {}
                continue;
            } else if (pid == 0) {
                // In child process, compile file
                if (chdir(studentFolderPath) == -1) {
                    if (write(1, "Error in: chdir", strlen("Error in: chdir")) == -1) {}
                    exit(-1);
                }
                if (dup2(fd, STDERR_FILENO) == -1) {
                    if (write(1, "Error in: dup2", strlen("Error in: dup2")) == -1) {}
                    exit(-1);
                }
                gccRet = execlp("gcc", "gcc", entry->d_name, NULL);
                if (gccRet == -1) {
                    if (write(1, "Error in: execlp", strlen("Error in: execlp")) == -1) {}
                    exit(-1);
                }
            } else {
                // Wait for child process to finish
                int waited = wait(&gccStatus);
                if (waited == -1) {
                    if (write(1, "Error in: wait", strlen("Error in: wait")) == -1) {}
                    continue;
                }
                // It is assumed that there is only one C file in a folder
                break;
            }
        }
    }
        char resultStr[MAX_LINE_LENGTH];
        strcpy(resultStr, folder->d_name);
        //Case where no c file was in the folder
        if (fileFound == 0) {
            strcat(resultStr, ",0,NO_C_FILE\n");
            if (write(result, resultStr, strlen(resultStr)) == -1) {}
            if (closedir(dir) == -1) {
                if (write(1, "Error in: closedir", strlen("Error in: closedir")) == -1) {}
                exit(-1);
            }
            continue;
        }

        //Iterating again over the student's directory to find an a.out file
        if (closedir(dir) == -1) {
            if (write(1, "Error in: closedir", strlen("Error in: closedir")) == -1) {}
            exit(-1);
        }
        if ((dir = opendir(studentFolderPath)) == NULL) {
            if (write(1,"Error in: opendir", strlen("Error in: opendir")) == -1) {}
            continue;
        }
        int hasCompiled = 0;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, "a.out") == 0) {
                //If an a.out file was found it means that there was no compilation error
                hasCompiled = 1;
                //Running the a.out file with the input text and writing the results to text file
                int executeStatus, executeReturnCode;
                pid_t executePid;
                executePid = fork();
                if (executePid == -1) {
                    if (write(1, "Error in: fork", strlen("Error in: fork")) == -1) {}
                    continue;
                }
                    //The a.out file will be run in the child process
                else if (executePid == 0) {
                    if ((userFD = open(USER_OUTPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
                        if (write(1, "Error in: open", strlen("Error in: open")) == -1) {}
                        exit(-1);
                    }
                    //Changing path to the file's direction
                    if (chdir(studentFolderPath) == -1) {
                        if (write(1, "Error in: chdir", strlen("Error in: chdir")) == -1) {}
                        exit(-1);
                    }
                    //Redirecting needed IO
                    if (dup2(fd, 2) == -1) {
                        if (write(1, "Error in: dup2", strlen("Error in: dup2")) == -1) {}
                        exit(-1);
                    }
                    if (dup2(input, 0) == -1) {
                        if (write(1, "Error in: dup2", strlen("Error in: dup2")) == -1) {}
                        exit(-1);
                    }
                    if (dup2(userFD, 1) == -1) {
                        if (write(1, "Error in: dup2", strlen("Error in: dup2")) == -1) {}
                        exit(-1);
                    }
                    //Making sure the input file will be read from the beginning
                    if (lseek(input, 0, SEEK_SET) == -1) {
                        if (write(1, "Error in: lseek", strlen("Error in: lseek")) == -1) {}
                        exit(-1);
                    }
                    executeReturnCode = execlp("./a.out", "./a.out", NULL);
                    if (executeReturnCode == -1) {
                        if (write(1, "Error in: execlp", strlen("Error in: execlp")) == -1) {}
                        exit(-1);
                    }
                    close(userFD);
                } else {
                    int waited = wait(&executeStatus);
                    if (waited == -1) {
                        if (write(1, "Error in: wait", strlen("Error in: wait")) == -1) {}
                        continue;
                    }


                    //Removing the a.out file in the child process
                    int removeStatus;
                    pid_t removePid;
                    removePid = fork();
                    if (removePid == -1) {
                        if (write(1, "Error in: fork", strlen("Error in: fork")) == -1) {}
                        continue;
                    } else if (removePid == 0) {
                        //Changing path to the file's direction
                        if (chdir(studentFolderPath) == -1) {
                            if (write(1, "Error in: chdir", strlen("Error in: chdir")) == -1) {}
                            exit(-1);
                        }
                        int removeReturnCode = execlp("rm", "rm", "a.out", NULL);
                        if (removeReturnCode == -1) {
                            if (write(1, "Error in: execlp", strlen("Error in: execlp")) == -1) {}
                            exit(-1);
                        }
                    } else {
                        int removeWait = wait(&removeStatus);
                        if (removeWait == -1) {
                            if (write(1, "Error in: wait", strlen("Error in: wait")) == -1) {}
                            continue;
                        }

                    }
                }
                //Comparing the student's output with the expected output in the child process
                int compareStatus, compareReturnCode;
                pid_t comparePid;
                comparePid = fork();
                if (comparePid == -1) {
                    if (write(1, "Error in: fork", strlen("Error in: fork")) == -1) {}
                    continue;
                } else if (comparePid == 0) {
                    if (dup2(fd, 2) == -1) {
                        if (write(1, "Error in: dup2", strlen("Error in: dup2")) == -1) {}
                        exit(-1);
                    }
                    compareReturnCode = execlp("./comp.out", "./comp.out", correctOutput, USER_OUTPUT_FILE, NULL);
                    if (compareReturnCode == -1) {
                        if (write(1, "Error in: execlp", strlen("Error in: execlp")) == -1) {}
                        exit(-1);
                    }
                } else {
                    int waited = wait(&compareStatus);
                    if (waited == -1) {
                        if (write(1, "Error in: wait", strlen("Error in: wait")) == -1) {}
                        continue;
                    }

                    //Giving a grade depending on the result of the comparison
                    int compare = WEXITSTATUS(compareStatus);
                    if (compare == 1) {
                        strcat(resultStr, ",100,EXCELLENT\n");
                        if (write(result, resultStr, strlen(resultStr)) == -1) {}
                    } else if (compare == 3) {
                        strcat(resultStr, ",75,SIMILAR\n");
                        if (write(result, resultStr, strlen(resultStr)) == -1) {}
                    } else if (compare == 2) {
                        strcat(resultStr, ",50,WRONG\n");
                        if (write(result, resultStr, strlen(resultStr)) == -1) {}
                    }
                }
            }
        }
        //Compilation error case
        if (hasCompiled == 0) {
            strcat(resultStr, ",10,COMPILATION_ERROR\n");
            if (write(result, resultStr, strlen(resultStr)) == -1) {}
        }
        //Closing the directory of the student
        if (closedir(dir) == -1) {
            if (write(1, "Error in: closedir", strlen("Error in: closedir")) == -1) {}
            exit(-1);
        }
    }

    //Closing the main folder and all other files
    if (closedir(studentsDir) == -1) {
        if (write(1, "Error in: closedir", strlen("Error in: closedir")) == -1) {}
        exit(-1);
    }
    close(input);
    close(correctOutput);
    close(fd);
    close(result);



}