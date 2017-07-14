/* -*- mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#include "mem_map.h"

void abort_msg(int code, char *fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
    exit(code);
}

/* A signal handler, so PRU can be stopped on operator Ctrl-C. */

int signal_exit = 0;
void sig_handler(int signo) {
    if (signo == SIGINT)
        signal_exit = 1;
}


int pru_wait_event_nonblock_by_pru(int pru)
{
    int host_event = (pru==0 ? PRU_EVTOUT_0 : PRU_EVTOUT_1);
    unsigned int event_count = -1;
    int fd = prussdrv_pru_event_fd(host_event);
    /* Studies show that (n_read, event_count, errno) are either:
       (-1, unset, 11)           <- no interrupt waiting
       ( 4, event_count, unset)  <- yes interrupt waiting.
    */
    read(fd, &event_count, sizeof(int));
    return event_count;
}

int pru_clear_event(int pru)
{
    int host_event = (pru==0 ? PRU_EVTOUT_0 : PRU_EVTOUT_1);
    int arm_int = (pru==0 ? PRU0_ARM_INTERRUPT : PRU1_ARM_INTERRUPT);
    prussdrv_pru_clear_event(host_event, arm_int);
    return 0;
}

#define N_PRU 2
typedef struct {
    void *mem[N_PRU];
    void *shared_mem;
} pru_t;

int init_prus(pru_t *pru)
{
    static tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    prussdrv_init();

    int evts[] = {PRU_EVTOUT_0, PRU_EVTOUT_1, -1};
    for (int evti=0; evts[evti]>=0; evti++) {
        if (prussdrv_open(evts[evti]) != 0)
            abort_msg(10, "prussdrv_open open failed for "
                      "event %i\n", evts[evti]);

        int host_int = evts[evti];
        int fd = prussdrv_pru_event_fd(host_int);
        int flags = fcntl(fd, F_GETFL, 0);
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0)
            abort_msg(10, "Failed to set host interrupt %i "
                      "to be non-blocking; errno=%i\n",
                      evts[evti], errno);
    }
    prussdrv_pruintc_init(&pruss_intc_initdata);

    prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, &pru->mem[0]);
    prussdrv_map_prumem(PRUSS0_PRU1_DATARAM, &pru->mem[1]);
    prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &pru->shared_mem);

    return 0;
}





int main (void)
{
    int err = 0;

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        abort_msg(10, "Failed to install signal handler\n");

    /* Initialize PRU library with non-blocking event fds. */
    pru_t pru;
    init_prus(&pru);

    /* Map the PRU memory into a local data structure.  Initialize any
       values that need to be set prior to starting the PRU program. */
    volatile
    main_mem_t *main_mem = (pru.mem[THIS_PRU_INDEX] + MAINMEM_ADDR);
    memset((void*)main_mem, 0, sizeof(*main_mem));

    // First goal is 3e6...
    main_mem->target = 3000000;

    /* Start the PRU. */
    const char* fw_reader = "test0.bin";
    err = prussdrv_exec_program(THIS_PRU_INDEX, fw_reader);
    if (err != 0)
        abort_msg(10, "Execute reader returned %i.\n", err);

    /* Start the monitoring loop. */
    const int step_size_us = 100000;
    const int max_iter = 10;
    int iter = 0;

    while (iter < max_iter && !signal_exit) {

        if (iter == 50)
            main_mem->exit_request = 1;

        printf("Loop iteration %i: target=%i, counter=%i\n",
               iter, main_mem->target, main_mem->counter);
        usleep(step_size_us);

        /* Did PRU signal us? */
        int message = pru_wait_event_nonblock_by_pru(THIS_PRU_INDEX);
        if (message >= 0) {
            pru_clear_event(THIS_PRU_INDEX);
            printf("signal(%i) = %i\n", message, main_mem->signal);
            if (main_mem->signal == 2) // exit.
                break;
            if (main_mem->signal == 1) {
                main_mem->counter = 0;
                main_mem->target = 6000000;
            }
        }
        iter++;

    }


    printf("Shutting down PRU.\n");
    prussdrv_pru_disable(THIS_PRU_INDEX);
    prussdrv_exit();

    return(0);

}


