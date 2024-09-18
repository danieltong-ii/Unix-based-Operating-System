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

#include "globals.h"
#include "errno.h"
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off, void **ret)
{

    //     uint32_t start_addr;

    //size_t start_addr;
    //int error = -EINVAL;

    // PARAM CHECK: Make sure that only one flag is set
    if (!((flags & MAP_TYPE) == MAP_SHARED) && !((flags & MAP_TYPE) == MAP_PRIVATE))
    {
        dbg(DBG_PRINT, "(GRADING3D 1)\n");
        return -EINVAL;
    }

    // PARAM CHECK: make sure that the offset is aligned to page boundary, important
    if (!PAGE_ALIGNED(off))
    {
        dbg(DBG_PRINT, "(GRADING3D 1)\n");
        return -EINVAL;
    }
    // PARAM CHECK: Make sure that len is correct, inside boundary
    if (len <= 0 || len > (USER_MEM_HIGH - USER_MEM_LOW))
    {
        dbg(DBG_PRINT, "(GRADING3D 1)\n");
        return -EINVAL;
    }
    if (!PAGE_ALIGNED(addr))
    {
        dbg(DBG_PRINT, "(GRADING3D 1)\n");
        return -EINVAL;
    }

    size_t start_addr;

    // PARAM CHECK: Checking the flags
    if ((flags & MAP_FIXED))
    {
        if ((uint32_t)addr < USER_MEM_LOW || ((uint32_t)addr + len) > USER_MEM_HIGH)
        {
            dbg(DBG_PRINT, "(GRADING3D 1)\n"); 
            return -EINVAL;
        }
        start_addr = ADDR_TO_PN(addr);
        dbg(DBG_PRINT, "(GRADING3D 1)\n");
    }
    else
    {
        start_addr = 0; // start_addr not used if MAP_FIXED isn't used, let vmmap_map find the page
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    dbg(DBG_PRINT, "(GRADING3A)\n"); 

    // FILE HANDLING and MAPPING
    file_t *file = NULL;
    vmarea_t *vmarea = NULL;
    int retval = 0;
    vnode_t *vnode = NULL;

    size_t len_last = (uint32_t)PAGE_ALIGN_UP(len);
    size_t npages = len_last / PAGE_SIZE;

    // make sure that the flag is not ANON
    if (!(flags & MAP_ANON))
    {

        if (fd== -1) {
            dbg(DBG_PRINT, "(GRADING3D 1)\n");
            return -EBADF;
        }

        file = fget(fd);
        if (file == NULL)
        {
            dbg(DBG_PRINT, "(GRADING3D 1)\n");
            return -EBADF;
        }

        if (((flags & MAP_SHARED) && (prot & PROT_WRITE) && !(file->f_mode & FMODE_WRITE)))
        {
            fput(file);
            dbg(DBG_PRINT, "(GRADING3D 1)\n");
            return -EACCES;
        }
        vnode = file->f_vnode;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }

    if (file)
    {
        fput(file);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }

    int vmmap_ret_val = vmmap_map(curproc->p_vmmap, vnode, start_addr, npages, prot, flags, off, VMMAP_DIR_HILO, &vmarea);
    if (vmmap_ret_val < 0)
    {
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        return vmmap_ret_val;
    }

    if (ret)
    {
        *ret = PN_TO_ADDR(vmarea->vma_start);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }

    //error = vmmap_map(curproc->p_vmmap, vnode, start_addr, npages, prot, flags, off, VMMAP_DIR_HILO, &vmarea);
    //*ret = PN_TO_ADDR(vmarea->vma_start);

    pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(vmarea->vma_start), (uintptr_t)PN_TO_ADDR(vmarea->vma_start + npages));
    tlb_flush_range((uintptr_t)PN_TO_ADDR(vmarea->vma_start), npages);

    KASSERT(NULL != curproc->p_pagedir); /* page table must be valid after a memory segment is mapped into the address space */
    dbg(DBG_PRINT, "(GRADING3A 2.a)\n");
    return vmmap_ret_val;
}


/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{

    if (len == 0 || (uint32_t)addr < USER_MEM_LOW || (uint32_t)addr + len > USER_MEM_HIGH)
    {
        dbg(DBG_PRINT, "(GRADING3D 1)\n");
        return -EINVAL;
    }

    if (!PAGE_ALIGNED(addr) || len <= 0 || len > (USER_MEM_HIGH - USER_MEM_LOW))
    {
        dbg(DBG_PRINT, "(GRADING3D 1)\n");
        return -EINVAL;
    }

    unsigned int lopage = ADDR_TO_PN(addr);
    size_t npages = ((uint32_t)PAGE_ALIGN_UP(len)) / PAGE_SIZE;

    int retval = vmmap_remove(curproc->p_vmmap, lopage, npages);

    pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(lopage + npages));

    tlb_flush_range((uintptr_t)PN_TO_ADDR(lopage), npages);
    dbg(DBG_PRINT, "(GRADING3D 1)\n");

    return 0; //retval;
}

