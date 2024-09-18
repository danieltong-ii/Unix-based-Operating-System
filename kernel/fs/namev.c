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
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
        if(len == 0){
            vref(dir);
            *result= dir;
            dbg(DBG_PRINT, "(GRADING2B)\n");
            return 0;
        }
        KASSERT(NULL != dir); /* the "dir" argument must be non-NULL */
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
        
        KASSERT(NULL != name);
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
        int return_value = dir->vn_ops->lookup(dir, name, len, result);


        KASSERT(NULL != result);
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");

        if(name[len] == '/' && return_value == 0){
            if(!S_ISDIR((*result)->vn_mode)){
                vput(*result);
                *result = NULL;
                return_value = -ENOTDIR;
                dbg(DBG_PRINT, "(GRADING2B)\n");
            }
            dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return return_value;

}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int
dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
        vnode_t* curparent = NULL; 

    KASSERT(NULL != pathname);
    dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
    // If the pathname is empty, return an error.
    if (pathname[0] == '\0'){
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return -EINVAL; 
    }

    // Check if the pathname does not start with '/', meaning it's relative to the current directory or base.
    if (!(pathname[0] == '/')) {
            curparent = curproc->p_cwd; // Assign current working directory as the parent.
            vref(curparent); // Increase the reference count of the parent vnode.
            dbg(DBG_PRINT, "(GRADING2B)\n");
            
        
    } else {
        // If the pathname starts with '/', it's an absolute path; start from the root.
        curparent = vfs_root_vn; // Assign the root vnode as the parent.
        vref(curparent); // Increase the reference count of the root vnode.
        dbg(DBG_PRINT, "(GRADING2B)\n");
    }


    // Calculate the length of the pathname, excluding any trailing slashes.
    int path_len = strlen(pathname)-1;
    int result = 0;

    // Trim trailing slashes from the pathname.
    for (; path_len >= 0; path_len--) {
        if (pathname[path_len] != '/') {
            dbg(DBG_PRINT, "(GRADING2B)\n");
            break;
        }
    }

    int begin = 0; // Start index of the current component.
    int end = 0; // End index of the current component.
    int file_length = 0; // Length of the current component.
    int flag_1 = 0; // Flag to mark when a valid file or directory name is found.
    int index = 0; // Index for iterating through the pathname.

    vnode_t* curr_file = NULL;
    KASSERT(NULL != curparent); // CHECK AGAIN
    dbg(DBG_PRINT, "(GRADING2A 2.b)\n") ;

    // Parse the pathname to identify the file or directory names.
    for (index = 0; pathname[index] != '\0' && index <= path_len; index++) {
        if (pathname[begin] != '/') {
            begin += 1; // Move the begin index forward if not a slash.
            flag_1 = 1; // Mark that a valid name component has started.
            file_length = begin - end; // Calculate the current name length.
            dbg(DBG_PRINT, "(GRADING2B)\n");
        } else {
            if (flag_1 == 1) {
                // Lookup the current component within the parent directory.
                if ((result = lookup(curparent, &pathname[end], file_length, &curr_file)) != 0) {
                    vput(curparent); // Release the parent vnode on failure.
                    dbg(DBG_PRINT, "(GRADING2B)\n");
                    return result; // Return the lookup result.
                }
                flag_1 = 0; // Reset flag after successful lookup.
                vput(curparent); // Release the parent vnode.
                curparent = curr_file; // Set the current file as the new parent.
                dbg(DBG_PRINT, "(GRADING2B)\n");

            }
            begin += 1; // Move past the slash.
            end = begin; // Set end to the new starting point.
            dbg(DBG_PRINT, "(GRADING2B)\n");

        }
    }

    // Final check for name length exceeding the limit.
    if (NAME_LEN < file_length) {
        vput(curparent); // Release the parent vnode.
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return -ENAMETOOLONG; // Return error for name too long.
    }

    // Adjust the end index if the entire pathname has been processed.
    if (path_len == -1) {
        end = strlen(pathname); // Set end to the length of the pathname.
        dbg(DBG_PRINT, "(GRADING2B)\n");
    }

    KASSERT(NULL != namelen);
    dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
    KASSERT(NULL != name);
    dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
    KASSERT(NULL != res_vnode);
    dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

    *name = &pathname[end]; // Set the name to the last component found.
    *namelen = file_length; // Set the name length.
    *res_vnode = curparent; // Set the resulting vnode to the current parent.

    dbg(DBG_PRINT, "(GRADING2B)\n");

    return 0; 
}


/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified and the file does
 * not exist, call create() in the parent directory vnode. However, if the
 * parent directory itself does not exist, this function should fail - in all
 * cases, no files or directories other than the one at the very end of the path
 * should be created.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        
    vnode_t *target_vnode; 
    int dir_flag = 0;
    size_t namelen; 
    const char* name; 

    // Call dir_namev to split the path into directory and name components, also resolving the directory vnode.
    int result = dir_namev(pathname, &namelen, &name, base, &target_vnode);
    // Check the result of dir_namev; if it's an error code, return it immediately.
    if (result != 0){
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return result;
    }

    // Check if the character at the position of the name length in 'name' is a slash, indicating a directory.
    if (name[namelen] == '/') {
        dir_flag = 1; // Set the directory flag.
        dbg(DBG_PRINT, "(GRADING2B)\n");
    }

    // Try to find the vnode for the name component in the path within the target directory vnode.
    if ((result = lookup(target_vnode, name, namelen, res_vnode)) != 0) {

        if ((flag & O_CREAT)) { /* to create a file, need to make sure that the directory vnode supports the create operation */
        KASSERT(NULL != target_vnode->vn_ops->create);
        dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
        dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
        dbg(DBG_PRINT, "(GRADING2B)\n");

        }
        // If lookup failed because the file doesn't exist and O_CREAT flag is set, try to create the file or directory.
        if ((flag & O_CREAT) && (result == -ENOENT)) { 
            // If not looking for a directory (based on dir_flag), attempt to create the file.
            if (!dir_flag) {
                result = target_vnode->vn_ops->create(target_vnode, name, namelen, res_vnode);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                // If creation fails, release the target vnode and return the error code.
                if (result != 0) {
                    vput(target_vnode);
                    dbg(DBG_PRINT, "(GRADING2B)\n");
                    return result;
                }

            }

        }

       }
    // After processing, release the target vnode.
    vput(target_vnode);
    dbg(DBG_PRINT, "(GRADING2B)\n");
    return result;
}


#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
