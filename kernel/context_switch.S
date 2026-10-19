@ Context switch for Cortex-M33. Task code runs on PSP; exceptions run on MSP.
        .syntax unified
        .cpu cortex-m33
        .thumb

@ void kernel_start_first(uint32_t *sp)    r0 = initial saved sp of first task
        .global kernel_start_first
        .thumb_func
kernel_start_first:
        ldmia   r0!, {r4-r11}       @ drop the (zeroed) software-saved regs
        msr     psp, r0             @ PSP now points at the hardware frame
        ldr     r2, =current_task
        ldr     r2, [r2]
        ldr     r2, [r2, #4]        @ this task's stack limit
        msr     psplim, r2          @ CPU faults if PSP ever drops below it
        movs    r1, #2
        msr     control, r1         @ thread mode uses PSP
        isb
        pop     {r0-r3}             @ build the first task's registers by hand
        pop     {r12}
        pop     {lr}                @ task_exit
        pop     {r1}                @ PC -> entry function
        add     sp, sp, #4          @ skip xPSR
        bx      r1

@ PendSV: save current task, pick next, restore it.
@ Hardware already pushed r0-r3, r12, lr, pc, xPSR onto the task's PSP stack.
        .global isr_pendsv
        .thumb_func
isr_pendsv:
        mrs     r0, psp
        ldr     r1, =current_task
        ldr     r2, [r1]
        ldr     r3, [r2, #4]        @ stack limit of the task being switched out
        subs    r12, r0, #32        @ where PSP will be after pushing r4-r11
        cmp     r12, r3
        blo     overflow            @ would run past the stack: stop
        stmdb   r0!, {r4-r11}       @ save the registers hardware didn't
        str     r0, [r2]            @ tcb->sp = r0

        bl      kernel_pick_next    @ current_task = next

        ldr     r1, =current_task
        ldr     r2, [r1]
        ldr     r0, [r2]            @ r0 = new tcb->sp
        ldmia   r0!, {r4-r11}
        movs    r3, #0
        msr     psplim, r3          @ drop the old limit so setting PSP can't trip it
        msr     psp, r0
        ldr     r3, [r2, #4]
        msr     psplim, r3          @ limit for the task we are switching in
        mvn     lr, #2              @ EXC_RETURN 0xFFFFFFFD: thread mode, use PSP
        bx      lr                  @ hardware pops r0-r3,r12,lr,pc,xPSR
overflow:
        b       kernel_stack_overflow   @ records the fault and never returns
        .ltorg
