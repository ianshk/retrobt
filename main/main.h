#ifndef MAIN_H
#define MAIN_H

#include <inttypes.h>

uint16_t request_send_control();
int poll_send_control();
void reset_send_control();

uint16_t request_send_interrupt();
int poll_send_interrupt();
void reset_send_interrupt();

#endif
