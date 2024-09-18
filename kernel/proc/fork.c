/******************************************************************************/
/* Important Spring 2024 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{

    KASSERT(regs != NULL); /* the function argument must be non-NULL */
    KASSERT(curproc != NULL);
    KASSERT(curproc->p_state == PROC_RUNNING);
    dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

    vmarea_t *vma, *clone_vma;
    pframe_t *pf;
    mmobj_t *to_delete, *new_shadowed;

    // Create a new process structure for the child.
    proc_t *child_proc = proc_create("child_proc");
    KASSERT(child_proc->p_state == PROC_RUNNING);
    KASSERT(child_proc->p_pagedir != NULL);
    dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
    mmobj_t *pshadow, *bottom_object;

    // Iterate over all VM areas in the child's virtual memory map.
    //vmarea_t *iter_vma;
    list_iterate_begin(&child_proc->p_vmmap->vmm_list, clone_vma, vmarea_t, vma_plink) {
        // Find the corresponding VMA in the parent's map.
        vma = vmmap_lookup(curproc->p_vmmap, clone_vma->vma_start);
        to_delete = vma->vma_obj;
        // Handle based on whether the VMA is private or shared.
        if ((vma->vma_flags & MAP_TYPE) == MAP_PRIVATE) {
            bottom_object = mmobj_bottom_obj(to_delete);

            pshadow = shadow_create();
            pshadow->mmo_shadowed = to_delete;
            pshadow->mmo_un.mmo_bottom_obj = bottom_object;
            bottom_object->mmo_ops->ref(bottom_object);

            new_shadowed = shadow_create();
            new_shadowed->mmo_shadowed = to_delete;
            new_shadowed->mmo_un.mmo_bottom_obj = bottom_object;
            bottom_object->mmo_ops->ref(bottom_object);

            // Assign shadow objects to parent and child VMAs.
            vma->vma_obj = pshadow;
            clone_vma->vma_obj = new_shadowed;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        } else {
            // For shared mappings, increment the reference count.
            clone_vma->vma_obj = to_delete;
            dbg(DBG_PRINT, "(GRADING3D 1)\n");
        }
        // Link the new vmarea to the list of vmareas managed by the memory object.
        to_delete->mmo_ops->ref(to_delete);
        list_insert_tail(mmobj_bottom_vmas(clone_vma->vma_obj), &clone_vma->vma_olink);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    } list_iterate_end();

    dbg(DBG_PRINT, "(GRADING3A)\n");

    // Clone the current thread to create a thread for the child process.
    kthread_t *child_thread = kthread_clone(curthr);
    KASSERT(child_thread->kt_kstack != NULL);
    dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
    // Setup the child thread context for execution.

    regs->r_eax = 0;
    child_thread->kt_ctx.c_esp = fork_setup_stack(regs, child_thread->kt_kstack);
    child_thread->kt_ctx.c_eip = (uint32_t) userland_entry;

    sched_make_runnable(child_thread);

    // Reset user space mappings and flush TLB
    pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
    tlb_flush_all();

    // Make the child process ready to run

    dbg(DBG_PRINT, "(GRADING3A)\n");
    return child_proc->p_pid;  // Parent process sees this return value
}
