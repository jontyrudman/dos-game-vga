#ifndef TIMER_H
#define TIMER_H

extern volatile unsigned long fast_tick, slow_tick;

static void interrupt (far *oldtimer)();
void deinit_timer();
void init_timer();

#endif
