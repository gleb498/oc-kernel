#include <arch/reg.h>
#include <arch/idt.h>
#include <sched/task.h>
#include <sched/sched.h>
#include <utils/kprint.h>
#include <utils/kpanic.h>
#include <utils/kassert.h>
#include <lib/string.h>
#include <lib/time.h>
#include <lib/stdtypes.h>
#include <messages.h>

static struct sched_task_t *current_task; /* current running process */

/*
 * Api - Init
 */
extern void sched_init() {
  /* no task currently running */
  current_task = null;
  /* init task module */
  task_init();
}

/*
 * Api - Schedule task to run
 */
extern void sched_schedule(size_t *ret_addr, size_t *reg_addr) {
  struct sched_task_t *next_task = null;

  /* finish current task */
  if (current_task != null) {
    /* update running time */
    current_task->time += 1;

    /* check quota exceed */
    if (current_task->time < TASK_QUOTA && !current_task->reschedule) {
      return; /* continue task execution */
    }

    /* reset quota */
    current_task->time = 0;
    current_task->reschedule = false;

    /* save task state */
    current_task->op_registers.eip = *ret_addr;
    current_task->op_registers.cs = *(u16*)((size_t)ret_addr + 4);
    *(u32*)(&current_task->flags) = *(u32*)((size_t)ret_addr + 6);
    current_task->op_registers.esp = (size_t)ret_addr + 10;
    current_task->gp_registers.esp = current_task->op_registers.esp;
    memcpy(&current_task->gp_registers, (void*)reg_addr, sizeof(current_task->gp_registers));
  }

  /* pick next task */
  next_task = task_find_to_run(current_task);
  kassert(__FILE__, __LINE__, next_task != null);

  /* prepare context for the next task */
  *(u32*)(next_task->op_registers.esp - 4) = *(u32*)(&next_task->flags);
  *(u16*)(next_task->op_registers.esp - 6) = next_task->op_registers.cs;
  *(u32*)(next_task->op_registers.esp - 10) = next_task->op_registers.eip;
  next_task->op_registers.esp = next_task->op_registers.esp - 10;
  next_task->gp_registers.esp = next_task->op_registers.esp;
  next_task->op_registers.esp = next_task->op_registers.esp - sizeof(next_task->gp_registers);
  memcpy((void*)next_task->op_registers.esp, &next_task->gp_registers, sizeof(next_task->gp_registers));
  
  /* update current task pointer */
  kprint("scheduled tid=%u sp=%X pc=%X->%X\n", next_task->tid, ret_addr, *ret_addr, next_task->op_registers.eip);
  // delay(5);
  current_task = next_task;

  /* run next task */
  asm_switch_context(next_task->op_registers.esp);
}

/*
 * Api - Get current running task
 */
extern struct sched_task_t *sched_get_current_task() {
  kassert(__FILE__, __LINE__, current_task != null);
  return current_task;
}

/*
 * Api - Yield current process
 */
extern void sched_yield() {
  struct sched_task_t *task;
  /* get current task */
  task = sched_get_current_task();
  /* mark task to be rescheduled */
  task->reschedule = true;
  /* reschedule */
  asm_int(INT_TIMER);
}
