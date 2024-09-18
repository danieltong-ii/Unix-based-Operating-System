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
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"

/*
 * This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page. Make sure that if the
 * user writes to the page it will be handled correctly. This
 * includes your shadow objects' copy-on-write magic working
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{
    //Initialize flags and error code
    uint32_t pt_flags = PT_PRESENT | PT_USER;
    uint32_t pd_flags = PD_PRESENT | PD_USER;
    // Set default error code to EFAULT
    int fault_error = EFAULT;
    int write_mode = 0;
    uint32_t v_frame_num = ADDR_TO_PN(vaddr);
    vmarea_t *vm_area = NULL;
    pframe_t *page_frame = NULL;
    // Check if the address is in the user memory range
    if (!(USER_MEM_LOW <= vaddr && vaddr < USER_MEM_HIGH)) {
        dbg(DBG_PRINT, "(GRADING3C 5)\n");
        do_exit(fault_error);
    }
    // Check if the address is page aligned
    vm_area = vmmap_lookup(curproc->p_vmmap, v_frame_num);
    if (!vm_area) {
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        do_exit(fault_error);
    }
    // Check if the address is in the vmarea
    if ((cause & FAULT_WRITE) && !(vm_area->vma_prot & PROT_WRITE)) {
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        do_exit(fault_error);
    }
    //
    if (vm_area->vma_prot == PROT_NONE ||
        ((!((cause & FAULT_WRITE) || (cause & FAULT_EXEC))) && (!(vm_area->vma_prot & PROT_READ)))) {
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        do_exit(fault_error);
    }

    if ((cause & FAULT_WRITE) != 0) {
        
        write_mode = 1;
        pt_flags |= PT_WRITE;
        pd_flags |= PD_WRITE;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }

    // Check if the page is in the page frame
    if (pframe_lookup(vm_area->vma_obj, vm_area->vma_off + (v_frame_num - vm_area->vma_start), write_mode, &page_frame) < 0) {
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        do_exit(fault_error);
    }

    KASSERT(page_frame);
    KASSERT(page_frame->pf_addr);
    dbg(DBG_PRINT, "(GRADING3A 5.a)\n");
    
    // Check if the page is dirty
    if (write_mode != NULL) {
        
        // If the page is dirty, pin it,  dirty it, and unpin it
        pframe_pin(page_frame);
        pframe_dirty(page_frame);
        pframe_unpin(page_frame);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    
    // Map the page to the page table
    uintptr_t paddr = pt_virt_to_phys((uintptr_t)page_frame->pf_addr);
    fault_error = pt_map(curproc->p_pagedir, (uintptr_t)PAGE_ALIGN_DOWN(vaddr), paddr, pd_flags, pt_flags);
    dbg(DBG_PRINT, "(GRADING3A)\n");
}
