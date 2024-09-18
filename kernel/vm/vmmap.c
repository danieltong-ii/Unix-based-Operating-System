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

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_create");
        vmmap_t *fresh_vmmap = (vmmap_t *)slab_obj_alloc(vmmap_allocator);

        if (fresh_vmmap)
        {
                list_init(&fresh_vmmap->vmm_list);
                fresh_vmmap->vmm_proc = NULL;
                dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return fresh_vmmap;
}
/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
    KASSERT(NULL != map); /* function argument must not be NULL */
    dbg(DBG_PRINT, "(GRADING3A 3.a)\n");

    vmarea_t *vm_area;

    list_iterate_begin(&map->vmm_list, vm_area, vmarea_t, vma_plink)
    {
        if (vm_area->vma_obj)
        {
            vm_area->vma_obj->mmo_ops->put(vm_area->vma_obj);
            vm_area->vma_obj = NULL;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        vm_area->vma_vmmap = NULL;

        if (list_link_is_linked(&(vm_area->vma_plink)))
        {
            list_remove(&(vm_area->vma_plink));
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }

        if (list_link_is_linked(&(vm_area->vma_olink)))
        {
            list_remove(&(vm_area->vma_olink));
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        vmarea_free(vm_area);
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    list_iterate_end();

    map->vmm_proc = NULL;
    slab_obj_free(vmmap_allocator, map);
    dbg(DBG_PRINT, "(GRADING3A)\n");
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
    KASSERT(NULL != map && NULL != newvma); /* both function arguments must not be NULL */
    KASSERT(NULL == newvma->vma_vmmap); /* newvma must be newly create and must not be part of any existing vmmap */
    KASSERT(newvma->vma_start < newvma->vma_end); /* newvma must not be empty */
    KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
    dbg(DBG_PRINT, "(GRADING3A 3.b)\n");

    // NOT_YET_IMPLEMENTED("VM: vmmap_insert");
    // Set the map of the new area
    newvma->vma_vmmap = map;

    // Declare variables
    vmarea_t *curr_area;

    // Iterate through the list of VM areas in the map
    list_iterate_begin(&map->vmm_list, curr_area, vmarea_t, vma_plink) {
        // Assign the condition to a variable
        int is_overlap = (curr_area->vma_start >= newvma->vma_end );

        // Check if there's an overlap between the new area and the current area
        if (is_overlap) {
            // Insert the new area before the current area
            list_insert_before(&curr_area->vma_plink, &newvma->vma_plink);
            // Exit the loop since insertion is done
            dbg(DBG_PRINT, "(GRADING3A)\n");
            return;
        }
        
    
        dbg(DBG_PRINT, "(GRADING3A)\n");
      // End of the iteration
    }list_iterate_end();

    // If the loop completes without insertion, insert the new area at the tail of the list
    list_insert_tail(&map->vmm_list, &newvma->vma_plink);
    dbg(DBG_PRINT, "(GRADING3A)\n");
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
        // return -1;
        //Set the lower and higher bound of the address space
         uint32_t lower_bound_vfn = ADDR_TO_PN(USER_MEM_LOW);
        uint32_t higher_bound_vfn = ADDR_TO_PN(USER_MEM_HIGH);
        uint32_t previous_vfn;
       
        vmarea_t *vma;
        // If no such range exists, return -1
        int start_vfn = -1;

        
        //If dir is VMMAP_DIR_HILO, find the highest gap in the address space
        if(dir == VMMAP_DIR_HILO){
                previous_vfn = higher_bound_vfn;
                list_iterate_reverse(&map->vmm_list, vma, vmarea_t, vma_plink){
                        // If the gap is big enough, return the start vfn
                        if ((previous_vfn - vma->vma_end) >= npages) {
                                start_vfn = previous_vfn - npages;
                                dbg(DBG_PRINT, "(GRADING3A)\n");
                                return start_vfn;
                        }
                        previous_vfn = vma->vma_start;
                        dbg(DBG_PRINT, "(GRADING3D 2)\n");
                }list_iterate_end();
                // If the gap is big enough, return the start vfn
                if((previous_vfn - lower_bound_vfn) >= npages){
                        start_vfn = previous_vfn - npages;
                        dbg(DBG_PRINT, "(GRADING3D 2)\n");
                }
                dbg(DBG_PRINT, "(GRADING3D 2)\n");
        }
        dbg(DBG_PRINT, "(GRADING3D 2)\n");
        return start_vfn;
}


/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
    KASSERT(NULL != map); /* the first function argument must not be NULL */
    dbg(DBG_PRINT, "(GRADING3A 3.c)\n");

        vmarea_t *vmarea;
        list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {
            if (vfn >= vmarea->vma_start) {
                if (vfn < vmarea->vma_end) {
                    dbg(DBG_PRINT, "(GRADING3A)\n");
                    return vmarea;
                }
                dbg(DBG_PRINT, "(GRADING3A)\n");
            }
        dbg(DBG_PRINT, "(GRADING3A)\n");
    } list_iterate_end();
    dbg(DBG_PRINT, "(GRADING3D 2)\n");
    return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        
    vmarea_t *current_area;
    vmarea_t *newvma;
    vmmap_t *memory_map = vmmap_create();
    

    if (memory_map)
    {
        list_iterate_begin(&map->vmm_list, current_area, vmarea_t, vma_plink)
        {
            newvma = vmarea_alloc();
            if (newvma)
            {
                memcpy(newvma, current_area, sizeof(vmarea_t));
        
                newvma->vma_vmmap = NULL;
                newvma->vma_obj = NULL;
                list_link_init(&newvma->vma_plink);
                list_link_init(&newvma->vma_olink);

                vmmap_insert(memory_map, newvma);
                dbg(DBG_PRINT, "(GRADING3A)\n");
            }
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        list_iterate_end();
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return memory_map;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
    KASSERT(NULL != map); /* must not add a memory segment into a non-existing vmmap */
    KASSERT(0 < npages); /* number of pages of this memory segment cannot be 0 */
    KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags)); /* must specify whether the memory segment is shared or private */
    KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage)); /* if lopage is not zero, it must be a user space vpn */
    KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages))); /* if lopage is not zero, the specified page range must lie completely within the user space */
    KASSERT(PAGE_ALIGNED(off)); /* the off argument must be page aligned */
    dbg(DBG_PRINT, "(GRADING3A 3.d)\n");

         vmarea_t *vm_area;
         vm_area = vmarea_alloc();

         int vm_error = 0;

         if(lopage == 0){
             vm_error = vmmap_find_range(map, npages, dir);

             if(vm_error < 0){
                 vmarea_free(vm_area);
                 dbg(DBG_PRINT, "(GRADING3D 2)\n");
                 return vm_error;
             }

             lopage = vm_error;
             vm_error = 0;
             dbg(DBG_PRINT, "(GRADING3A)\n");
         }

         vm_area->vma_start = lopage;
         vm_area->vma_end = lopage + npages;
    
         vm_area->vma_off = ADDR_TO_PN(off);
         vm_area->vma_flags = flags;
         vm_area->vma_prot = prot;

    
         vm_area->vma_obj = NULL;
    
         list_link_init(&vm_area->vma_plink);
         list_link_init(&vm_area->vma_olink);

         mmobj_t* vmmobj = NULL;

         if(!file){
             vmmobj = anon_create();
             dbg(DBG_PRINT, "(GRADING3A)\n");
         }
         else
         {
             vm_error = file->vn_ops->mmap(file, vm_area, &vmmobj);
             dbg(DBG_PRINT, "(GRADING3A)\n");
         }

         if (flags & MAP_PRIVATE){
                 mmobj_t *mmo_shadow = shadow_create();

                 mmo_shadow->mmo_shadowed = vmmobj;
                 mmo_shadow->mmo_un.mmo_bottom_obj = vmmobj;
            
                 vmmobj->mmo_ops->ref(vmmobj);
                 vm_area->vma_obj = mmo_shadow;
                 dbg(DBG_PRINT, "(GRADING3A)\n");
         }
         else
         {
                 vm_area->vma_obj = vmmobj;
                 dbg(DBG_PRINT, "(GRADING3D 2)\n");
         }

         if(!vmmap_is_range_empty(map, lopage, npages)){
             vm_error = vmmap_remove(map, lopage, npages);
             dbg(DBG_PRINT, "(GRADING3A)\n");
         }

         list_insert_tail(mmobj_bottom_vmas(vm_area->vma_obj), &(vm_area->vma_olink));
         vmmap_insert(map, vm_area);
         if(new){
                 *new = vm_area;
                dbg(DBG_PRINT, "(GRADING3A)\n");
         }
        dbg(DBG_PRINT, "(GRADING3A)\n");
         return 0;
 }


/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
     uint32_t initial_vfn, terminal_vfn;

     vmarea_t *memory_area;
     vmarea_t *new_memory_area;
     mmobj_t *vmmobj;

     uint32_t hipage = lopage + npages;

     list_iterate_begin(&map->vmm_list, memory_area, vmarea_t, vma_plink)
     {
         terminal_vfn = memory_area->vma_end;
         initial_vfn = memory_area->vma_start;
         vmmobj = memory_area->vma_obj;

         if (lopage > initial_vfn) {

             if (hipage < terminal_vfn) {

                 new_memory_area = vmarea_alloc();

                 if (new_memory_area)
                 { 
                        
                     memcpy(new_memory_area, memory_area, sizeof(vmarea_t));
                     new_memory_area->vma_vmmap = NULL;

                     list_link_init(&new_memory_area->vma_plink);
                     list_link_init(&new_memory_area->vma_olink);

                     new_memory_area->vma_off += hipage - initial_vfn;
                     new_memory_area->vma_obj = vmmobj;
                     new_memory_area->vma_start = hipage;
                     memory_area->vma_end = lopage;
                     vmmobj->mmo_ops->ref(vmmobj);

                     list_insert_tail(mmobj_bottom_vmas(new_memory_area->vma_obj), &(new_memory_area->vma_olink));
                     vmmap_insert(map, new_memory_area);
                     dbg(DBG_PRINT, "(GRADING3D 2)\n");
                 }
                 dbg(DBG_PRINT, "(GRADING3D 2)\n");
             }
             dbg(DBG_PRINT, "(GRADING3D 2)\n");
         }
        
         if (lopage <= initial_vfn) {
             if (hipage > initial_vfn) {
                 if (hipage < terminal_vfn) {
                     memory_area->vma_start = hipage;
                     memory_area->vma_off += hipage - initial_vfn;
                     dbg(DBG_PRINT, "(GRADING3D 2)\n");
                 }
                 dbg(DBG_PRINT, "(GRADING3D 2)\n");
             }
             dbg(DBG_PRINT, "(GRADING3D 2)\n");
         }
         
          if (lopage > initial_vfn) {
             if (lopage < terminal_vfn) {
                 if (hipage >= terminal_vfn) {
                     memory_area->vma_end = lopage;
                     dbg(DBG_PRINT, "(GRADING3D 2)\n");
                 }
                 dbg(DBG_PRINT, "(GRADING3D 2)\n");
             }
             dbg(DBG_PRINT, "(GRADING3D 2)\n");
         }
         
          if (lopage <= initial_vfn) {
             if (hipage >= terminal_vfn) {

                 if (memory_area->vma_obj)
                 {
                     memory_area->vma_obj->mmo_ops->put(memory_area->vma_obj);
                     memory_area->vma_obj = NULL;
                     dbg(DBG_PRINT, "(GRADING3A)\n");

                 }
                 memory_area->vma_vmmap = NULL;

                 if (list_link_is_linked(&(memory_area->vma_plink)))
                 {
                     list_remove(&(memory_area->vma_plink));
                     dbg(DBG_PRINT, "(GRADING3A)\n");
                 }
                 if (list_link_is_linked(&(memory_area->vma_olink)))
                 {
                     list_remove(&(memory_area->vma_olink));
                     dbg(DBG_PRINT, "(GRADING3A)\n");
                 }
                 vmarea_free(memory_area);
                 dbg(DBG_PRINT, "(GRADING3A)\n");
             }
             dbg(DBG_PRINT, "(GRADING3A)\n");
         }
         dbg(DBG_PRINT, "(GRADING3A)\n");
     } list_iterate_end();

     dbg(DBG_PRINT, "(GRADING3A)\n");
     return 0;
 }

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");

        uint32_t endvfn = startvfn + npages;
        vmarea_t *vma;

        KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
        dbg(DBG_PRINT, "(GRADING3A 3.e)\n");

        //Iterate through the list of vmareas in the given address space
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                //Check if there are mappings for the given address space
                if(!((vma->vma_start >= endvfn) || (vma->vma_end <= startvfn))){
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        return 0; //return zero if there are
                }
                dbg(DBG_PRINT, "(GRADING3A)\n");
        } list_iterate_end();

        dbg(DBG_PRINT, "(GRADING3A)\n");
        return 1; //if the given address space has no mappings
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
    int error = 0;
    size_t bytesRead = 0;
    size_t bytesToRead = 0;
    void *targetBuffer = buf;
    uint32_t startOffset = PAGE_OFFSET(vaddr);
    uint32_t endOffset = PAGE_OFFSET((uintptr_t)vaddr + count);
    uint32_t startPage = ADDR_TO_PN(vaddr);
    uint32_t endPage = ADDR_TO_PN(PAGE_ALIGN_UP((uintptr_t)vaddr + count));
    uint32_t currentPage = startPage;
    uint32_t currentOffset = 0;
    vmarea_t *area;
    pframe_t *frame;
    uint32_t pageNumber;

    // Processing each virtual page
    while (currentPage < endPage && bytesRead < count) {
        // Locate corresponding vmarea for the current page
        area = vmmap_lookup(map, currentPage);
        // Fetch the physical frame for the calculated page number
        error = pframe_lookup(area->vma_obj, area->vma_off + (currentPage - area->vma_start), 0, &frame);

        // Deciding the amount of data to read from the current page
    if (currentPage == startPage) {
        bytesToRead = PAGE_SIZE - startOffset;
        if (count < bytesToRead) {
            bytesToRead = count;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        currentOffset = startOffset;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    } else if (currentPage == (endPage - 1)) {
        bytesToRead = endOffset;
        currentOffset = 0;
        dbg(DBG_PRINT, "(GRADING3D 1)\n");
    }

        // Copying data from the physical frame to the user buffer
        memcpy(targetBuffer, (const void *)((uintptr_t)frame->pf_addr + currentOffset), bytesToRead);
        // Advance the buffer pointer, update the read count and proceed to next page
        targetBuffer = (void *)((uintptr_t)targetBuffer + bytesToRead);
        bytesRead += bytesToRead;
        currentPage++;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return error;
}


/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
    // NOT_YET_IMPLEMENTED("VM: vmmap_write");
    int error = 0;
    uint32_t start_vfn = ADDR_TO_PN(vaddr);
    uint32_t end_vfn = ADDR_TO_PN(PAGE_ALIGN_UP(((uintptr_t)vaddr + count)));
    const void *current_buf = buf;
    uintptr_t start_offset = PAGE_OFFSET(vaddr);
    uintptr_t end_offset = PAGE_OFFSET((uintptr_t)vaddr + count);
    uint32_t current_vfn = start_vfn;
    uintptr_t buf_offset = 0;
    size_t total_written = 0;
    size_t bytes_to_write = 0;
    vmarea_t *area = NULL;
    pframe_t *frame = NULL;
    uint32_t page_number;

    while (end_vfn > current_vfn && count > total_written) {
        // Lookup the virtual memory area for the current virtual frame number
        area = vmmap_lookup(map, current_vfn);
        // Lookup the physical frame associated with this page number
        error = pframe_lookup(area->vma_obj, area->vma_off + (current_vfn - area->vma_start), 1, &frame);
        // if (error) {
        //     dbg(DBG_PRINT, "(GRADING3A)\n");
        //     return error; // Return on failure
        // }

        // Determine the number of bytes to write at this stage
        if (current_vfn == start_vfn) {
            bytes_to_write = PAGE_SIZE - start_offset;
            bytes_to_write = (count < bytes_to_write) ? count : bytes_to_write;
            buf_offset = start_offset;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        } else if (current_vfn == end_vfn - 1) {
            bytes_to_write = end_offset;
            buf_offset = 0;
            dbg(DBG_PRINT, "(GRADING3D 1)\n");
        } 
        // else {
        //     bytes_to_write = PAGE_SIZE;
        //     buf_offset = 0;
        //     dbg(DBG_PRINT, "(GRADING3A)\n");
        // }

        // Operations on the physical frame
        pframe_pin(frame);
        pframe_dirty(frame);
        pframe_unpin(frame);

        // Copy data from the source buffer to the physical frame
        memcpy((void *)((uintptr_t)frame->pf_addr + buf_offset), current_buf, bytes_to_write);
        // Update buffer pointer, total bytes written, and current virtual frame number
        current_buf = (const void *)((uintptr_t)current_buf + bytes_to_write);
        total_written += bytes_to_write;
        current_vfn++;
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return error;
}
