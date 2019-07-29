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
 * @file uuardopd.h
 * @author Rafael Diniz
 * @date 12 Apr 2018
 * @brief Rhizo HF Connector main header file
 *
 * Rhizo HF Connector main header file.
 *
 */

#ifndef HAVE_UUARDOPD_H__
#define HAVE_UUARDOPD_H__

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

// 30s
#define TIMEOUT_DEFAULT 30

#define BUFFER_SIZE 4096

typedef struct{
// modem related data
    char call_sign[32];
    char remote_call_sign[32];
    int tcp_base_port;
    char ip_address[32];
    char modem_type[32];
    bool ofdm_mode;
    int timeout;
    int data_socket;
    int control_socket;

// uuport pipe filenames (calling)
    char input_pipe[1024];
    char output_pipe[1024];

    uint32_t buffer_size;

// pipes fd which are used to talk to uucico (receiving a call)
    int pipefd1[2];
    int pipefd2[2];

// state variables
// C11 atomic is used here instead of a more pedantic code with mutexes and so on... 
    atomic_bool tcp_ret_ok;
    atomic_bool connected;
    atomic_bool waiting_for_connection;
    atomic_bool clean_buffers;
    atomic_int timeout_counter; // only for VARA
    atomic_int safe_state; // this means green light for changing state, only for VARA

    atomic_int session_counter_read;
    atomic_int session_counter_write;

// uuardopd private buffers
    buffer in_buffer;
    buffer out_buffer;
} rhizo_conn;

#ifdef __cplusplus
};
#endif

#endif /* HAVE_UUARDOPD_H__ */