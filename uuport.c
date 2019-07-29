/* Rhizo-uuardop: Tools to integrate Ardop to UUCP
 * Copyright (C) 2019 Rhizomatica
 * Author: Rafael Diniz <rafael@riseup.net>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 */

/**
 * @file uuport.c
 * @author Rafael Diniz
 * @date 17 Jul 2019
 * @brief UUCP port pipe command
 *
 * UUPORT main C file.
 *
 */

#include <sys/ioctl.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include "uuport.h"

FILE *log_fd;

void *read_thread(void *file_name_v)
{
    char *file_name = file_name_v;
    uint8_t buffer[BUFFER_SIZE];
    bool running = true;
    int bytes_pipe = 0;
    int bytes_read = 0;
    int bytes_written = 0;

    int input_fd = open(file_name, O_RDONLY);

    if (input_fd == -1)
    {
        fprintf(log_fd, "read_thread: Error opening: %s\n", file_name);
        return NULL;
    }

    while (running)
    {
        ioctl(input_fd, FIONREAD, &bytes_pipe);
        if (bytes_pipe > BUFFER_SIZE)
            bytes_pipe = BUFFER_SIZE;
        if (bytes_pipe == 0)
            bytes_pipe = 1;

        bytes_read = read(input_fd, buffer, bytes_pipe);

        if (bytes_read == -1)
        {
            fprintf(log_fd, "read_thread: read() error! error no: %d\n",errno);
            running = false;
            continue;
        }
        if (bytes_read == 0)
        {
            fprintf(log_fd, "read_thread: read == 0\n");
            running = false;
            continue;
        }

        bytes_written = write(1, buffer, bytes_read);

        if (bytes_written != bytes_read)
        {
            fprintf(log_fd, "read_thread: bytes_written: %d != bytes_read: %d.\n", bytes_written, bytes_read);
            running = false;
            continue;
        }
        if (bytes_written == -1)
        {
            fprintf(log_fd, "read_thread: write() error no: %d\n", errno);
            running = false;
            continue;
        }

    }

    close(input_fd);
    return NULL;

}

void *write_thread(void *file_name_v)
{
    char *file_name = file_name_v;
    uint8_t buffer[BUFFER_SIZE];
    bool running = true;
    int bytes_read = 0;
    int bytes_written = 0;

    int output_fd = open(file_name, O_WRONLY);

    if (output_fd == -1)
    {
        fprintf(log_fd, "write_thread: Error opening: %s\n", file_name);
        return NULL;
    }

    while(running)
    {
        bytes_read = read(0, buffer, BUFFER_SIZE);

        if (bytes_read == -1)
        {
            fprintf(log_fd, "write_thread: Error in write(), errno: %d\n", errno);
            running = false;
            continue;
        }
        if (bytes_read == 0)
        {
            fprintf(log_fd, "write_thread: read() returned 0\n");
            continue;
        }

        bytes_written = write(output_fd, buffer, bytes_read);

        if (bytes_written == -1)
        {
            if (errno == EPIPE)
            {
                fprintf(log_fd, "write_thread: write() EPIPE!\n");
            }
            else
            {
                fprintf(log_fd, "write_thread: write() error no: %d\n", errno);

            }
            running = false;
            continue;
        }
        if (bytes_written != bytes_read)
        {
            fprintf(log_fd, "write_thread: bytes_written: %d !=  bytes_read: %d\n", bytes_written, bytes_read);
        }
    }

    close(output_fd);
    return NULL;
}

void finish(int s){

    if (s == SIGINT)
        fprintf(log_fd, "\nSIGINT: Exiting.\n");

    if (s == SIGTERM)
        fprintf(log_fd, "\nSIGTERM: Exiting.\n");

    if (s == SIGQUIT)
        fprintf(log_fd, "\nSIGQUIT: Exiting.\n");

    if (s == SIGHUP)
        fprintf(log_fd, "\nSIGHUP: Exiting.\n");

    if (s == SIGPIPE)
        fprintf(log_fd, "\nSIGPIPE: Exiting\n");


    // some house keeping here?

    fclose(log_fd);
    exit(EXIT_SUCCESS);
}


int main (int argc, char *argv[])
{
    char input_pipe[BUFFER_SIZE];
    char output_pipe[BUFFER_SIZE];
    char log_file[BUFFER_SIZE];
    log_file[0] = 0;

    signal (SIGINT, finish);
    signal (SIGTERM, finish);
    signal (SIGQUIT, finish);
    signal (SIGHUP, finish);

    signal (SIGPIPE, finish);

//    signal (SIGPIPE, SIG_IGN); // ignores SIGPIPE...

    fprintf(stderr, "Rhizomatica's uuport version 0.1 by Rafael Diniz -  rafael (AT) rhizomatica (DOT) org\n");
    fprintf(stderr, "License: GPLv3+\n\n");

    if (argc < 4)
    {
    manual:
        fprintf(stderr, "Usage modes: \n%s -i input_pipe -o output_pipe -l logfile\n", argv[0]);
        fprintf(stderr, "%s -h\n", argv[0]);
        fprintf(stderr, "\nOptions:\n");
        fprintf(stderr, " -i input_pipe.uucp           Data to be written to uucico.\n");
        fprintf(stderr, " -o output_pipe.uucp          Data read from uucico.\n");
        fprintf(stderr, " -l logfile.txt               Log file (default is stderr).\n");
        fprintf(stderr, " -h                           Prints this help.\n");
        exit(EXIT_FAILURE);
    }

    int opt;
    while ((opt = getopt(argc, argv, "i:o:hl:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            goto manual;
            break;
        case 'i':
            strcpy(input_pipe, optarg);
            break;
        case 'o':
            strcpy(output_pipe, optarg);
            break;
        case 'l':
            strcpy(log_file, optarg);
            break;
        default:
            goto manual;
        }
    }

    if (log_file[0])
    {
        log_fd = fopen(log_file, "a");
        if (log_fd == NULL)
        {
            fprintf(stderr, "Log file could not be opened: %s\n", log_file);
            fprintf(stderr, "Reverting to stderr log.\n");
            log_fd = stderr;
        }
    }
    else
    {
        log_fd = stderr;
    }

    pthread_t tid;

    pthread_create(&tid, NULL, read_thread, (void *) input_pipe);

//    pthread_create(&tid, NULL, write_thread, (void *) &connector);
    write_thread(output_pipe);

    return EXIT_SUCCESS;
}
