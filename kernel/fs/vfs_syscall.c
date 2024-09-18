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

/*
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.2 2018/05/27 03:57:26 cvsps Exp $
 */

#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/*
 * Syscalls for vfs. Refer to comments or man pages for implementation.
 * Do note that you don't need to set errno, you should just return the
 * negative error code.
 */

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read vn_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int
do_read(int fd, void *buf, size_t nbytes)
{
        file_t *file;
        int return_value;

        if (fd == -1) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        file = fget(fd);

        if (!file) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        if(!((file->f_mode) & FMODE_READ)){
               fput(file); 
               dbg(DBG_PRINT, "(GRADING2B)\n");
               return -EBADF;
        }

        if (!(file->f_vnode->vn_ops->read)){
                fput(file);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EISDIR;
        }

        //Call its virtual read vn_op
        return_value = file->f_vnode->vn_ops->read(file->f_vnode, file->f_pos, buf, nbytes);
        //Update f_pos
        if(return_value > 0){

                file->f_pos += return_value;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        
        fput(file);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        //Return the number of bytes read, or an error
        return return_value;
}


/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * vn_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int
do_write(int fd, const void *buf, size_t nbytes)
{
        file_t *file;
        int err;

        //Check if the file is writable
        if (fd == -1) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        file = fget(fd);

        if (!file) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        
        //Check if the file is open for writing
        if(!((file->f_mode) & FMODE_WRITE)){
               fput(file); 
               dbg(DBG_PRINT, "(GRADING2B)\n");
               return -EBADF;
        }

        // Check if the file is a directory (Is this needed?)
        if (!(file->f_vnode->vn_ops->write)){
                fput(file);
                dbg(DBG_PRINT, "(GRADING2C 1)\n");  // NOT HIT 2B
                return -EISDIR;
        } //REVOKED

	if (file->f_mode & FMODE_APPEND){
                if((err = do_lseek(fd, 0, SEEK_END)) < 0){
                        fput(file);
                        dbg(DBG_TEMP, "(GRADING2A)\n");
                        return err;
                }
                //err = do_lseek(fd, 0, SEEK_END);
                dbg(DBG_PRINT, "(GRADING2B)\n"); // TEST
        }
    
        //Call the write vn_op
        err = file->f_vnode->vn_ops->write(file->f_vnode, file->f_pos, buf, nbytes);
        if(err > 0){
                file->f_pos += err;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        KASSERT((S_ISCHR(file->f_vnode->vn_mode)) ||
             (S_ISBLK(file->f_vnode->vn_mode)) ||
             ((S_ISREG(file->f_vnode->vn_mode)) && (file->f_pos <= file->f_vnode->vn_len))); /* cursor must not go past end of file for these file types */
 		
 		dbg(DBG_PRINT, "(GRADING2A 3.a)\n");

        //Decrement the reference count of the file
        fput(file);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return err;
}


/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int
do_close(int fd)
{
        // NOT_YET_IMPLEMENTED("VFS: do_close");
        // return -1;
        // Declare pointer to a file_t structure
        file_t *file_pointer;
        // Retrieve the file associated with the file descriptor & check if its valid
        if (fd == -1) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        file_pointer = fget(fd);

        // Decrement the reference count of the file
        if(!file_pointer){
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        fput(file_pointer);

        // Decrement the reference count of the file associated with the file descriptor
        fput(curproc->p_files[fd]);
        // Set the file associated with the file descriptor in the current process to NULL
        curproc->p_files[fd] = NULL;
        dbg(DBG_PRINT, "(GRADING2B)\n");
        // Return zero on success
        return 0;
}

/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int
do_dup(int fd)
{
        // NOT_YET_IMPLEMENTED("VFS: do_dup");
        // return -1;

        // Declare pointer to a file_t structure
        file_t *file;

        if(fd==-1){
                return -EBADF;
        }

        file = fget(fd);

        file_t *file_ret_value = file;

        if(!file_ret_value){
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        

        // Get an empty file descriptor for the current process and check if it is valid
        if ((fd = get_empty_fd(curproc)) != -EMFILE)
        {
                curproc->p_files[fd] = file;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        else
        {
                fput(file); // Decrement the reference count of the file
               // dbg(DBG_PRINT, "(GRADING2B)\n"); // NOT HIT 2B
		//NOT SURE!
        }
        // Return new file descriptor
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return fd;
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int
do_dup2(int ofd, int nfd)
{
        // NOT_YET_IMPLEMENTED("VFS: do_dup2");
        // return -1;

        // Declare pointer to a file_t structure
        file_t *open_file;
        int error;

        if (nfd < 0 || nfd >= NFILES)
        {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
            return -EBADF; // nfd is out of the allowed range for file descriptors
        }

        if (ofd == -1) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        open_file = fget(ofd); // Get the file associated with the given file descriptor
        if (!open_file) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        if (ofd != nfd) // If nfd is in use and not the same as ofd, close it
        {
                if ((curproc->p_files[nfd] != NULL))
                {
                        error = do_close(nfd);
                        if (error < 0)
                        {
                                fput(open_file);
                                dbg(DBG_PRINT, "(GRADING2B)\n"); // NOT HIT 28
                                return error;
                        //NOT SURE!
                        }
                }
                curproc->p_files[nfd] = open_file; // Point the new fd to the same file_t* as the given fd
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        else
        { // If nfd is the same as ofd
                fput(open_file);
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return nfd;
}


/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mknod(const char *path, int mode, unsigned devid)
{
        // NOT_YET_IMPLEMENTED("VFS: do_mknod");
        // return -1;

        // Check if the mode requested creation of something other than a device special file
        if (!S_ISCHR(mode) && !S_ISBLK(mode)){
        	dbg(DBG_PRINT, "(GRADING2B)\n"); // NOT HIT 2B
                return -EINVAL;
        }

        size_t name_length;
        const char* file_name;
        vnode_t *directory_vn;
        vnode_t *result_vn;
        int err;

        if ((err=dir_namev(path, &name_length, &file_name, NULL, &directory_vn)) == 0){
                if((err=lookup(directory_vn, file_name, name_length, &result_vn)) != 0){
                	    KASSERT(NULL != directory_vn->vn_ops->mknod);
                	    dbg(DBG_PRINT, "(GRADING2A 3.b)\n");
                        err = directory_vn->vn_ops->mknod(directory_vn, file_name, name_length, mode, (devid_t) devid);
                       // KASSERT(NULL != directory_vn->vn_ops->mknod);
                        vput(directory_vn);
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                else {
                        vput(directory_vn);
                        vput(result_vn);
                        err = -EEXIST;
                        //dbg(DBG_PRINT, "(GRADING2B)\n"); // NOT HIT 2B
                
                }

                dbg(DBG_PRINT, "(GRADING2B)\n");

        }


        dbg(DBG_PRINT, "(GRADING2B)\n");
        return err;
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mkdir(const char *path)
{
        // NOT_YET_IMPLEMENTED("VFS: do_mkdir");
        // return -1;

    size_t namelen;
    const char *name;
    vnode_t *parent_vnode;
    vnode_t *res_vnode_lookup;

    int mkdir_error = dir_namev(path, &namelen, &name, NULL, &parent_vnode);
    if (mkdir_error !=0){
    	dbg(DBG_PRINT, "(GRADING2B)\n"); 
            return mkdir_error;
    }
        //changed second arg from path to name 
    mkdir_error = lookup(parent_vnode, name, namelen, &res_vnode_lookup);



        if(mkdir_error == -ENOENT){

        		KASSERT(NULL != parent_vnode->vn_ops->mkdir);
        		dbg(DBG_PRINT, "(GRADING2A 3.c)\n");
                mkdir_error = parent_vnode->vn_ops->mkdir(parent_vnode, name, namelen);
                vput(parent_vnode);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                
        }else{
                vput(parent_vnode);
                
                if(mkdir_error >=0){
                	dbg(DBG_PRINT, "(GRADING2B)\n");
                    vput(res_vnode_lookup);
                    dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                mkdir_error = -EEXIST;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return mkdir_error;

       // NOT_YET_IMPLEMENTED("VFS: do_mkdir");
       // return -1;
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_rmdir(const char *path)
{
        size_t namelen;
        const char *name;
        vnode_t *parent_vnode;
        
        int do_rmdir_result = dir_namev(path, &namelen, &name, NULL, &parent_vnode);
        if (do_rmdir_result != 0)
        {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return do_rmdir_result;
        }
        if (namelen == 1 && name[0] == '.')
        {
                vput(parent_vnode);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }
        if (namelen == 2 && name[0] == '.' && name[1] == '.')
        {
                vput(parent_vnode);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOTEMPTY;
        }
        KASSERT(NULL != parent_vnode->vn_ops->rmdir);
        dbg(DBG_PRINT, "(GRADING2A 3.d)\n");
        dbg(DBG_PRINT, "(GRADING2A 3.d)\n");
        do_rmdir_result = parent_vnode->vn_ops->rmdir(parent_vnode, name, namelen);
        vput(parent_vnode); // check

        dbg(DBG_PRINT, "(GRADING2B)\n");
        return do_rmdir_result;
}

/*
 * Similar to do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EPERM
 *        path refers to a directory.
 *      o ENOENT
 *        Any component in path does not exist, including the element at the
 *        very end.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_unlink(const char *path)
{
    size_t namelen;
    const char *name;
    vnode_t *parent_vnode;
    vnode_t *res_vnode;
    int dir_namev_result = dir_namev(path, &namelen, &name, NULL, &parent_vnode);

    if (dir_namev_result != 0)
    {
    	dbg(DBG_PRINT, "(GRADING2B)\n");
        return dir_namev_result;
    }
    dir_namev_result = lookup(parent_vnode, name, namelen, &res_vnode);

    if (dir_namev_result != 0) //Handles -ENOENT and -ENOTDIR cases
    {
        vput(parent_vnode);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return dir_namev_result;
    }

    // If the lookup was successful but the vnode represents a directory, return EPERM.
    if (S_ISDIR(res_vnode->vn_mode))
    {
        vput(parent_vnode);
        vput(res_vnode);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return -EPERM;
    }

    // Perform the unlink operation.

    KASSERT(NULL != parent_vnode->vn_ops->unlink);
    dbg(DBG_PRINT, "(GRADING2A 3.e)\n");
    dbg(DBG_PRINT, "(GRADING2A 3.e)\n");

    dir_namev_result = parent_vnode->vn_ops->unlink(parent_vnode, name, namelen);

    vput(parent_vnode);
    vput(res_vnode); // Release the vnode of the file.

    dbg(DBG_PRINT, "(GRADING2B)\n");
    return dir_namev_result;
}


/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 *      o EPERM
 *        from is a directory.
 */
int
do_link(const char *from, const char *to)
{
       // NOT_YET_IMPLEMENTED("VFS: do_link");

        int status = 0;
        
        // struct stat buf;
        size_t namelen = 0;
        const char *name = NULL;
        vnode_t * old_vnode = NULL;
        vnode_t * dest_parent_dir = NULL;
        vnode_t * error_check_vnode = NULL;

        // STEP: Check if 'from' is a directory
        

        // STEP: Ensure 'from' vnode exists
        // ERRORS CHECKED: ENOENT, ENOTDIR, ENAMETOOLONG from lookup
        status = open_namev(from, 0, &old_vnode, NULL);
        if (status != 0) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
            return status;
        }

        if (S_ISDIR(old_vnode->vn_mode)) {
                vput(old_vnode);
                dbg(DBG_PRINT, "(GRADING2B)\n"); // NOT HIT 2B
                return -EPERM;
        }

        // STEP: Attempt retrieval of parent vnode of 'to'
        status = dir_namev(to, &namelen, &name, NULL, &dest_parent_dir);

        if (status != 0) {
            vput(old_vnode);
            dbg(DBG_PRINT, "(GRADING2B)\n"); 
            return status;
        }

        status = lookup(dest_parent_dir, name, namelen, &error_check_vnode);
        if (status == 0) {
                vput(error_check_vnode);
                status = -EEXIST;
                //dbg(DBG_PRINT, "(GRADING2B)\n"); // NOT HIT 2B
        }

        if (name[namelen] == '/') {
                status = -EPERM;
               // dbg(DBG_PRINT, "(GRADING2B)\n"); // NOT HIT 2B
        }

        if (status == -ENOENT) {
        	// KASSERT(NULL != parent_vnode->vn_ops->unlink); // NOT NEEDED
   			// dbg(DBG_PRINT, "(GRADING2A 3.e)\n") // NOT NEEDED
                status = dest_parent_dir->vn_ops->link(old_vnode, dest_parent_dir, name, namelen);
               
        }

      //   // Cleanup
        vput(old_vnode);
        vput(dest_parent_dir);

        dbg(DBG_PRINT, "(GRADING2B)\n"); // NOT HIT 2B
        return status;
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int
do_rename(const char *oldname, const char *newname)
{
        int link_ret = do_link(oldname, newname);
        if (0 <= link_ret)
        {
                link_ret = do_unlink(oldname);
        	dbg(DBG_PRINT, "(GRADING2B)\n"); 
               // return link_ret;
        }
        
        dbg(DBG_PRINT, "(GRADING2B)\n"); // NOT HIT 2B
        return link_ret;
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int
do_chdir(const char *path)
{
        size_t namelen;
        const char *name;
        vnode_t *res_vnode_lookup;

        int chdir_result  = open_namev(path,0, &res_vnode_lookup, NULL);
        if(chdir_result != 0){
        	dbg(DBG_PRINT, "(GRADING2B)\n");
            return chdir_result;
        }

        if(!S_ISDIR(res_vnode_lookup->vn_mode)){ 
        	dbg(DBG_PRINT, "(GRADING2B)\n");
            vput(res_vnode_lookup);
            return -ENOTDIR;
        }
        
        vput(curproc->p_cwd);
        curproc->p_cwd = res_vnode_lookup;

        dbg(DBG_PRINT, "(GRADING2B)\n");
        return 0;


       // NOT_YET_IMPLEMENTED("VFS: do_chdir");
        //return -1;
}

/* Call the readdir vn_op on the given fd, filling in the given dirent_t*.
 * If the readdir vn_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int
do_getdent(int fd, struct dirent *dirp)
{
       // NOT_YET_IMPLEMENTED("VFS: do_getdent");

        file_t *fileobj = NULL;
        int status = 0;

        // STEP: check if fd is valid file descriptor

        if (fd == -1){
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        fileobj = fget(fd);

        if (!fileobj) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
            return -EBADF;
        }


        // STEP: check if fd refers to a directory

        if (!(fileobj->f_vnode->vn_ops->readdir)){
                fput(fileobj);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOTDIR;
        }

        status = fileobj->f_vnode->vn_ops->readdir(fileobj->f_vnode, fileobj->f_pos, dirp);

        // STEP: fill in the given dirent
        if (status > 0) {
            //were given an offset
            fileobj->f_pos += status;
            status = sizeof(dirent_t);
        }
            fput(fileobj);
            dbg(DBG_PRINT, "(GRADING2B)\n");


        //dbg(DBG_PRINT, "(GRADING2B)\n");
        return status;
}


/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int
do_lseek(int fd, int offset, int whence)
{
        // NOT_YET_IMPLEMENTED("VFS: do_lseek");

        file_t *working_file;

        // STEP: If unsuccessful, return EBADF
        if (fd == -1){
        	dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        working_file = fget(fd);
        
        if (!working_file) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
            return -EBADF;
        }

        int new_position;


        if (whence == SEEK_SET) {
                new_position = offset;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        } else if (whence == SEEK_CUR) {
                new_position = working_file->f_pos + offset;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        } else if (whence == SEEK_END) {
                new_position = working_file->f_vnode->vn_len + offset;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        } else {
            fput(working_file);
            dbg(DBG_PRINT, "(GRADING2B)\n");
            return -EINVAL;
        }

        if (new_position < 0) {
            fput(working_file);
            dbg(DBG_PRINT, "(GRADING2B)\n");
            return -EINVAL;
        }

        // STEP: Release fd and return success
        working_file->f_pos = new_position;

        fput(working_file);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return new_position;
}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o EINVAL
 *        path is an empty string.
 */
int
do_stat(const char *path, struct stat *buf)
{
       // NOT_YET_IMPLEMENTED("VFS: do_stat");

        int status = 0;
        vnode_t * result_vnode = NULL;

        // STEP: Attempt retrieval of vnode
        status = open_namev(path, 0, &result_vnode, NULL);

        // STEP: Perform error checking
        if (status != 0) {
        	dbg(DBG_PRINT, "(GRADING2B)\n");
            return status;
        }
        // STEP: Perform stat system call
        
        KASSERT(NULL != result_vnode->vn_ops->stat);
    	dbg(DBG_PRINT, "(GRADING2A 3.f)\n");
    	dbg(DBG_PRINT, "(GRADING2A 3.f)\n");
        status = result_vnode->vn_ops->stat(result_vnode, buf);
        // STEP: Release vnode
        vput(result_vnode);

        dbg(DBG_PRINT, "(GRADING2B)\n");

        return status;
}

#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int
do_mount(const char *source, const char *target, const char *type)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
        return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int
do_umount(const char *target)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
        return -EINVAL;
}
#endif
