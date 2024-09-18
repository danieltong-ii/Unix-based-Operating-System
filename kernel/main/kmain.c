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

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "test/s5fs_test.h"

GDB_DEFINE_HOOK(initialized)

//Kernel 1
extern void *sunghan_test(int, void*);
extern void *sunghan_deadlock_test(int, void*);
extern void *faber_thread_test(int, void*);

//Kernel 2
extern void *vfstest_main(int, void*);
extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);

//kernel 3
extern int kprintf(kshell_t *ksh, const char *fmt, ...);

void      *bootstrap(int arg1, void *arg2);
void      *idleproc_run(int arg1, void *arg2);
kthread_t *initproc_create(void);
void      *initproc_run(int arg1, void *arg2);
void      *final_shutdown(void);

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
bootstrap(int arg1, void *arg2)
{
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        dbgq(DBG_TEST, "SIGNATURE: 53616c7465645f5f7e2586811a9dd1be5fd36f68beb3a6153e03869981b4ecf856964e995e11bc1eaf5ac79911b5f939\n");
        /* necessary to finalize page table information */
        pt_template_init();

        // NOT_YET_IMPLEMENTED("PROCS: bootstrap");
        proc_t *idleProc = proc_create("IDLEPROC");

        
        kthread_t *idleThread = kthread_create(idleProc, idleproc_run, 0, NULL);
     
        curproc = idleProc;
        curthr = idleThread;
        KASSERT(NULL != curproc);
        KASSERT(PID_IDLE == curproc->p_pid);
        KASSERT(NULL != curthr);
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");
        
        curthr->kt_state = KT_RUN;

        dbg(DBG_PRINT, "(GRADING1A)\n");
        context_make_active(&idleThread->kt_ctx);

        panic("weenix returned to bootstrap()!!! BAD!!!\n");
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;

        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

        #ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
        // NOT_YET_IMPLEMENTED("VFS: idleproc_run");

        if (!curproc->p_cwd) { // to do change dm
                curproc->p_cwd = vfs_root_vn; // Set current working directory of the process to the root directory of the virtual file system.
                vref(vfs_root_vn);
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        if (!initthr->kt_proc->p_cwd) {
                initthr->kt_proc->p_cwd = vfs_root_vn; // Set the initial thread's process' current working directory to the root directory of the virtual file system.
                vref(vfs_root_vn);
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        // NOT_YET_IMPLEMENTED("VFS: idleproc_run");

        //Make directory
        do_mkdir("/dev"); 

        //Make the devices
        do_mknod("/dev/null",S_IFCHR,MKDEVID(1,0)); //mode should either be S_IFCHR or S_IFBLK
        do_mknod("/dev/zero",S_IFCHR,MKDEVID(1,1)); 

        do_mknod("/dev/tty0",S_IFCHR,MKDEVID(2,0)); //first TTY device
        //do_mknod("/dev/tty1",S_IFCHR,MKDEVID(2,1)); //second TTY device ?


        #endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

        dbg(DBG_PRINT, "(GRADING2B)\n");
        return final_shutdown();
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
kthread_t *
initproc_create(void)
{
        proc_t *initProc = proc_create("INITPROC");
        KASSERT(NULL != initProc);
        KASSERT(PID_INIT == initProc->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");

        kthread_t *initThread = kthread_create(initProc, initproc_run, 0 , NULL);
        KASSERT(NULL != initThread);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");
        
        dbg(DBG_PRINT, "(GRADING1A)\n");
        return initThread;
}

#ifdef __DRIVERS__

        int cs402_faber_thread_test(kshell_t *kshell, int argc, char **argv)
        {
                KASSERT(kshell != NULL);
                dbg(DBG_TEMP, "(GRADING1C): cs402_faber_thread_test() is invoked, argc = %d, argv = 0x%08x\n",
                    argc, (unsigned int)argv);
                proc_t *childProc = proc_create("CHILDPROC");
                kthread_t *childThread = kthread_create(childProc, faber_thread_test, 0, NULL);

                sched_make_runnable(childThread);
                do_waitpid(childProc->p_pid, 0, NULL);

            /*
             * Must not call a test function directly.
             * Must create a child process, create a thread in that process and
             *     set the test function as the first procedure of that thread,
             *     then wait for the child process to die.
             */
            return 0;
        }

        int cs402_sunghan_test(kshell_t *kshell, int argc, char **argv)
        {
                KASSERT(kshell != NULL);
                dbg(DBG_TEMP, "(GRADING1D 1): cs402_sunghan_test() is invoked, argc = %d, argv = 0x%08x\n",
                    argc, (unsigned int)argv);
                proc_t *childProc = proc_create("CHILDPROC");
                kthread_t *childThread = kthread_create(childProc, sunghan_test , 0, NULL);

                sched_make_runnable(childThread);
                do_waitpid(childProc->p_pid, 0, NULL);

            /*
             * Must not call a test function directly.
             * Must create a child process, create a thread in that process and
             *     set the test function as the first procedure of that thread,
             *     then wait for the child process to die.
             */
            return 0;
        }

        int cs402_sunghan_deadlock_test(kshell_t *kshell, int argc, char **argv)
        {
                KASSERT(kshell != NULL);
                dbg(DBG_TEMP, "(GRADING1D 2): cs402_sunghan_deadlock_test() is invoked, argc = %d, argv = 0x%08x\n",
                    argc, (unsigned int)argv);
                proc_t *childProc = proc_create("CHILDPROC");
                kthread_t *childThread = kthread_create(childProc, sunghan_deadlock_test , 0, NULL);

                sched_make_runnable(childThread);
                do_waitpid(childProc->p_pid, 0, NULL);

            /*
             * Must not call a test function directly.
             * Must create a child process, create a thread in that process and
             *     set the test function as the first procedure of that thread,
             *     then wait for the child process to die.
             */
            return 0;
        }



#endif /* __DRIVERS__ */

#ifdef __VFS__

int cs_402_vfstest_main (kshell_t *kshell, int argc, char **argv)
{
        KASSERT(kshell != NULL);
        //dbg(DBG_TEMP, "(GRADING1D 2): cs402_sunghan_deadlock_test() is invoked, argc = %d, argv = 0x%08x\n",
                //   argc, (unsigned int)argv);
        proc_t *childProc = proc_create("VFSTEST");
        kthread_t *childThread = kthread_create(childProc, vfstest_main, argc, NULL);
        sched_make_runnable(childThread);
        do_waitpid(childProc->p_pid, 0, NULL);

            /*
             * Must not call a test function directly.
             * Must create a child process, create a thread in that process and
             *     set the test function as the first procedure of that thread,
             *     then wait for the child process to die.
             */
            return 0;
          
}

#endif /*__VFS__ */

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
initproc_run(int arg1, void *arg2)
{
        //Adding commands in the kshell to run all 3 tests
        #ifdef __DRIVERS__

        #ifdef __VM__
        char *const argv[] = {"/sbin/init", NULL };
        char *const envp[] = { NULL };
        dbg(DBG_PRINT, "GRADING3A)\n");
        kernel_execve("/sbin/init", argv, envp);
        panic("Returned to Init");
        #endif /* __VM__ */

        int err = 0;

        kshell_add_command("faber", cs402_faber_thread_test, "invoke faber thread test");
        kshell_add_command("sunghan", cs402_sunghan_test, "invoke sunghan test");
        kshell_add_command("sunghanlock", cs402_sunghan_deadlock_test, "invoke sunghan deadlock test");
        
        dbg(DBG_PRINT, "(GRADING1B)\n");

        #ifdef __VFS__
        kshell_add_command("thrtest", faber_fs_thread_test, "Run faber_fs_thread_test().");
        kshell_add_command("dirtest", faber_directory_test, "Run faber_directory_test().");
        kshell_add_command("vfstest", cs_402_vfstest_main, "Run vfstest_main().");

        #endif /* __VFS__ */

        kshell_t *kshell = kshell_create(0);
        if (NULL == kshell) panic("init: Couldn't create kernel shell\n");
        while ((err = kshell_execute_next(kshell))){
                if(err<0){
                        if (err == -ENOTDIR) {
                                kprintf(kshell, "Input path is not a directory\n");
                        }else{
                                kprintf(kshell, "Some issue with command error:%d\n", err);
                        }
                        dbg(DBG_ERROR, "kernel shell exited with :%d\n", err);
                }
        }

        kshell_destroy(kshell);


        /*RUNNING FABER TEST WITHOUT TEST 9*/

        #endif /* __DRIVERS__ */

        while(do_waitpid(-1,0,NULL) != -ECHILD);
        dbg(DBG_PRINT, "(GRADING1A)\n");

        return NULL;


}

