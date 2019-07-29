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
 * Ardop support routines
 */

/**
 * @file ardop.c
 * @author Rafael Diniz
 * @date 12 Apr 2018
 * @brief Ardop modem support functions
 *
 * All the specific code for supporting Ardop.
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "ardop.h"
#include "pipe.h"
#include "net.h"
#include "call_uucico.h"

void *ardop_data_worker_thread_tx(void *conn)
{
    rhizo_conn *connector = (rhizo_conn *) conn;
    uint8_t buffer[BUFFER_SIZE];
    int bytes_to_read;
    uint8_t ardop_size[2];
    uint32_t packet_size;
    bool running = true;

    while(running){

        // check if we are connected, otherwise, wait
        while (connector->connected == false || ring_buffer_count_bytes(&connector->in_buffer.buf) == 0){
            if (running == false){
                goto exit_local;
            }
            sleep(1);
        }

        bytes_to_read = ring_buffer_count_bytes(&connector->in_buffer.buf);
        if (bytes_to_read > BUFFER_SIZE)
            bytes_to_read = BUFFER_SIZE;
        if (bytes_to_read <= 0)
            bytes_to_read = 1;

        read_buffer(&connector->in_buffer, buffer, bytes_to_read);

        // fprintf(stderr, "ardop_data_worker_thread_tx: After read buffer\n");

        packet_size = bytes_to_read;

        uint32_t counter = 0;
        uint32_t tx_size = packet_size;
        while (tx_size != 0){

            if(connector->buffer_size > MAX_ARDOP_BUFFER){
                while (connector->buffer_size >  2* MAX_ARDOP_BUFFER / 3)
                    sleep(1);
            }

            if (tx_size > MAX_ARDOP_PACKET){
                ardop_size[0] = (uint8_t) (MAX_ARDOP_PACKET >> 8);
                ardop_size[1] = (uint8_t) (MAX_ARDOP_PACKET & 255);
            }
            else{
                ardop_size[0] = (uint8_t) (tx_size >> 8);
                ardop_size[1] = (uint8_t) (tx_size & 255);
            }

            // ardop header
            tcp_write(connector->data_socket, ardop_size, sizeof(ardop_size));

           // fprintf(stderr, "ardop_data_worker_thread_tx: After ardop header tcp_write\n");
            if (tx_size > MAX_ARDOP_PACKET)
            {
                tcp_write(connector->data_socket, &buffer[counter] , MAX_ARDOP_PACKET);
                counter += MAX_ARDOP_PACKET;
                tx_size -= MAX_ARDOP_PACKET;
            }
            else{
                tcp_write(connector->data_socket, &buffer[counter], tx_size);
                counter += tx_size;
                tx_size -= tx_size;
            }

            fprintf(stderr, "Tx bytes remaining: %u\n", tx_size);
            // buffer management hack
            sleep(2);
        }

    }

exit_local:

    fprintf(stderr, "Exiting ardop_data_worker_thread_tx... Do we really want this?\n");
    return EXIT_SUCCESS;
}

void *ardop_data_worker_thread_rx(void *conn)
{
    rhizo_conn *connector = (rhizo_conn *) conn;
    uint8_t buffer[MAX_ARDOP_PACKET_SAFE];
    uint32_t buf_size; // our header is 4 bytes long
    uint8_t ardop_size[2];

    while(connector->tcp_ret_ok){

        while (connector->connected == false){
            if (connector->tcp_ret_ok == false){
                goto exit_local;
            }
            sleep(1);
        }

        // fprintf(stderr,"Before tcp_read.\n");
        ardop_size[0] = 0; ardop_size[1] = 0;
        tcp_read(connector->data_socket, ardop_size, 2);

        // ARDOP TNC data format: length 2 bytes | payload
        buf_size = 0;
        buf_size = ardop_size[0];
        buf_size <<= 8;
        buf_size |= ardop_size[1];

        fprintf(stderr,"Ardop Rcv Pkt: %u bytes.\n", buf_size);

        tcp_read(connector->data_socket, buffer, buf_size);

        if (buf_size > 3 && !memcmp("ARQ", buffer,  3)){
            buf_size -= 3;
            write_buffer(&connector->out_buffer, buffer + 3, buf_size);
            // fprintf(stderr,"Buffer write: %u received.\n", buf_size);
            // fwrite(buffer+3, buf_size, 1, stdout);
        }
        else{
            buffer[buf_size] = 0;
            fprintf(stderr, "Ardop non-payload data rx: %s\n", buffer);
        }

    }

exit_local:
    return EXIT_SUCCESS;
}

void *ardop_control_worker_thread_rx(void *conn)
{
    rhizo_conn *connector = (rhizo_conn *) conn;
    uint8_t rcv_byte;
    uint8_t buffer[1024];
    int counter = 0;
    bool new_cmd = false;
    bool running = true;

    while(running){

        running &= tcp_read(connector->control_socket, &rcv_byte, 1);

        if (rcv_byte == '\r'){
            buffer[counter] = 0;
            counter = 0;
            new_cmd = true;
        }
        else{
            buffer[counter] = rcv_byte;
            counter++;
            new_cmd = false;
        }

// treat "STATUS CONNECT TO PP2UIT FAILED!" ?
// and reset waiting for connection!

        if (new_cmd){
            if (!memcmp(buffer, "DISCONNECTED", strlen("DISCONNECTED"))){
                fprintf(stderr, "TNC: %s\n", buffer);
                connector->connected = false;
                connector->waiting_for_connection = false;
            } else
            if (!memcmp(buffer, "NEWSTATE DISC", strlen("NEWSTATE DISC"))){
                fprintf(stderr, "TNC: %s\n", buffer);
                connector->connected = false;
                connector->waiting_for_connection = false;
            } else
            if (!memcmp(buffer, "CONNECTED", strlen("CONNECTED"))){
                fprintf(stderr, "TNC: %s\n", buffer);
                connector->connected = true;
                if (connector->waiting_for_connection == false)
                { // we are receiving a connection... call uucico!
                    bool retval = call_uucico(connector);
                    if (retval == false)
                        fprintf(stderr, "Error calling call_uucico()\n");
                }
                connector->waiting_for_connection = false;
            } else
            if (!memcmp(buffer, "PTT", strlen("PTT"))){
                // supressed output
                // fprintf(stderr, "%s -- CMD NOT CONSIDERED!!\n", buffer);
            } else
            if (!memcmp(buffer, "BUFFER", strlen("BUFFER"))){
                sscanf( (char *) buffer, "BUFFER %u", & connector->buffer_size);
                fprintf(stderr, "BUFFER: %u\n", connector->buffer_size);


            } else
                if (!memcmp(buffer, "INPUTPEAKS", strlen("INPUTPEAKS"))){
                    // suppressed output
                } else {
                    fprintf(stderr, "%s\n", buffer);
                }
        }
    }

    fprintf(stderr, "Leaving ardop_control_worker_thread_rx... Not good.\n");
    return EXIT_SUCCESS;
}

void *ardop_control_worker_thread_tx(void *conn)
{
    rhizo_conn *connector = (rhizo_conn *) conn;
    char buffer[1024];

    // initialize
    memset(buffer,0,sizeof(buffer));
    sprintf(buffer, "INITIALIZE\r");
    tcp_write(connector->control_socket, (uint8_t *) buffer, strlen(buffer));

    // We set a call sign
    memset(buffer,0,sizeof(buffer));
    sprintf(buffer, "MYCALL %s\r", connector->call_sign);
    tcp_write(connector->control_socket, (uint8_t *) buffer, strlen(buffer));

    // we take care of timeout, here we just set the wanted timeout + 5
    memset(buffer,0,sizeof(buffer));
    if (connector->timeout < 240)
        sprintf(buffer, "ARQTIMEOUT %d\r", connector->timeout);
    else
        sprintf(buffer, "ARQTIMEOUT %d\r", 240); // maximum timeout
    tcp_write(connector->control_socket, (uint8_t *) buffer, strlen(buffer));

    memset(buffer,0,sizeof(buffer));
    strcpy(buffer,"LISTEN True\r");
    tcp_write(connector->control_socket, (uint8_t *) buffer, strlen(buffer));

    memset(buffer,0,sizeof(buffer));
    strcpy(buffer,"BUSYDET 10\r");
    tcp_write(connector->control_socket, (uint8_t *) buffer, strlen(buffer));

    memset(buffer,0,sizeof(buffer));
    if (connector->ofdm_mode == true)
        strcpy(buffer,"ENABLEOFDM True\r");
    else
        strcpy(buffer,"ENABLEOFDM False\r");
    tcp_write(connector->control_socket, (uint8_t *) buffer, strlen(buffer));

    // 1Hz function
    while(connector->tcp_ret_ok){

        // Logic to start a connection
        if (connector->connected == false &&
            ring_buffer_count_bytes(&connector->in_buffer.buf) > 0 &&
            !connector->waiting_for_connection){

            memset(buffer,0,sizeof(buffer));
            sprintf(buffer,"ARQCALL %s 5\r", connector->remote_call_sign);
            tcp_write(connector->control_socket, (uint8_t *)buffer, strlen(buffer));

//            fprintf(stderr, "CONNECTING... %s\n", buffer);
            connector->waiting_for_connection = true;
        }

        if (connector->clean_buffers == true)
        {
            fprintf(stderr, "Connection closed - Cleaning internal buffers.\n");
            connector->clean_buffers = false;
            ring_buffer_clear (&connector->in_buffer.buf);
            ring_buffer_clear (&connector->out_buffer.buf);
            // clean up the buffers so we prevent data in the buffer which keeps trying to connect... and send DISCONNECT
        }

#if 0 // for debugging purposes
        // just calling buffer to help us...
        if (connector->connected == true){
            if (connector->timeout_counter % 2){
                memset(buffer,0,sizeof(buffer));
                sprintf(buffer, "BUFFER\r");
                tcp_write(connector->control_socket, (uint8_t *)buffer, strlen(buffer));
            }
        }
#endif

        sleep(1);

    }

    return EXIT_SUCCESS;
}

bool initialize_modem_ardop(rhizo_conn *connector){
    connector->tcp_ret_ok &= tcp_connect(connector->ip_address, connector->tcp_base_port, &connector->control_socket);
    connector->tcp_ret_ok &= tcp_connect(connector->ip_address, connector->tcp_base_port+1, &connector->data_socket);

    if (connector->tcp_ret_ok == false){
        fprintf(stderr, "Connection to TNC failure.\n");
        return false;
    }

    // we start our control rx thread
    pthread_t tid1;
    pthread_create(&tid1, NULL, ardop_control_worker_thread_rx, (void *) connector);

    // we start our control tx thread
    pthread_t tid2;
    pthread_create(&tid2, NULL, ardop_control_worker_thread_tx, (void *) connector);

    // and run the two workers for the data channel
    pthread_t tid3;
    pthread_create(&tid3, NULL, ardop_data_worker_thread_tx, (void *) connector);

    pthread_t tid4;
    pthread_create(&tid4, NULL, ardop_data_worker_thread_rx, (void *) connector);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);

    return true;
}