#ifndef __STUDENT_H__
#define __STUDENT_H__

#include "os-sim.h"

extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);

#endif