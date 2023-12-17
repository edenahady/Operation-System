// Eden Ahady 318948106

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0
#define BYTES_TO_READ 1024


int is_identical(int file1, int file2) {
    char buffer1[BYTES_TO_READ];
    char buffer2[BYTES_TO_READ];
    //making sure the files are read from the beginning
    int bytes_read1, bytes_read2;
    if (lseek(file1, 0, SEEK_SET) < 0) {
        if (write(1, "Error in: lseek", strlen("Error in: lseek")) == -1) {}
        exit(-1);
    }
    if (lseek(file2, 0, SEEK_SET) < 0) {
        if (write(1, "Error in: lseek", strlen("Error in: lseek")) == -1) {}
        exit(-1);
    }
    //Reading the bytes and comparing them
    do {
        bytes_read1 = read(file1, buffer1, BYTES_TO_READ);
        if (bytes_read1 < 0) {
            if (write(1, "Error in: read", strlen("Error in: read")) == -1) {}
            return -1;
        }
        bytes_read2 = read(file2, buffer2, BYTES_TO_READ);
        if (bytes_read2 < 0) {
            if (write(1, "Error in: read", strlen("Error in: read")) == -1) {}
            return -1;
        }

        //If both files have different length, so they are different
        if (bytes_read1 != bytes_read2) {
            return FALSE;
        }
        //Comparing the files byte by byte
        for (int i = 0; i < bytes_read1; i++) {
            if (buffer1[i] != buffer2[i]) {
                return FALSE;
            }
        }
    } while (bytes_read1 == BYTES_TO_READ);
    return TRUE;
}

int is_Similar(int file1, int file2) {
    char ch1 = ' ';
    char ch2 = ' ';
    int bytes_read1, bytes_read2;
    //making sure the files are read from the beginning
    if (lseek(file1, 0, SEEK_SET) == -1) {
        write(1, "Error in: lseek", strlen("Error in: lseek"));
        exit(-1);
    }
    if (lseek(file2, 0, SEEK_SET) == -1) {
        write(1, "Error in: lseek", strlen("Error in: lseek"));
        exit(-1);
    }
    //comparing the files to check if they are similar
    while (1) {
        //Reading one byte from each file, spaces and '\n' are ignored
        while ((bytes_read1 = read(file1, &ch1, 1)) > 0 && (ch1 == '\n' || ch1 == ' '));
        while ((bytes_read2 = read(file2, &ch2, 1)) > 0 && (ch2 == '\n' || ch2 == ' '));

        //Both files ended at the same time and all their corresponding chars were the same
        if (bytes_read1 == 0 && bytes_read2 == 0)
            break;

        //One file is shorter than the other when ignoring white spaces,they are different.
        if (bytes_read1 != bytes_read2) {
            return FALSE;
        }

        //Comparing one byte from each file.
        if (!(ch1 == ' ' || ch1 == '\n') && !(ch2 == ' ' || ch2 == '\n')) {
            if (toupper(ch1) != toupper(ch2)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}


int main(int argc, char *argv[]) {
    int file1, file2;
    int returnValue = 2;

    file1 = open(argv[1], O_RDONLY);
    if (file1 == -1) {
        if (write(1, "Error in: open", strlen("Error in: open")) == -1) {}
        return -1;
    }

    file2 = open(argv[2], O_RDONLY);
    if (file2 == -1) {
        if (write(1, "Error in: open", strlen("Error in: open")) == -1) {}
        close(file1);
        return -1;
    }

    int identical = is_identical(file1, file2);
    if (identical == TRUE) {
        returnValue = 1;
        goto cleanup;
    }

    int similar = is_Similar(file1, file2);
    if (similar == TRUE) {
        returnValue = 3;
        goto cleanup;
    }

    cleanup:
    if (lseek(file1, 0, SEEK_SET) == -1 || lseek(file2, 0, SEEK_SET) == -1) {
        if (write(1, "Error in: lseek", strlen("Error in: lseek")) == -1) {}
        returnValue = -1;
    }

    if (close(file1) == -1 || close(file2) == -1) {
        if (write(1, "Error in: close", strlen("Error in: close")) == -1) {}
        returnValue = -1;
    }

    return returnValue;
}

