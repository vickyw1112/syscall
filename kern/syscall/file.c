#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>
#include <proc.h>

/*
 * Add your file-related functions here ...
 */
struct oft_entry oft[OPEN_MAX];

int init_open_file_table(void){
    for(int i = 0; i < OPEN_MAX; i++){
        oft[i].entry_vnode = NULL;
        oft[i].fd_num = i;
        oft[i].entry_refcount = 1;
        oft[i].entry_offset = 0;
        oft[i].entry_lock = lock_create("entry_lock");
        if(oft[i].entry_lock == NULL){
            lock_destroy(oft[i].entry_lock);
            return ENOMEM;
        }
    }
    return 0;
}


int init_stdouterr(void){
    struct vnode *v1;
    struct vnode *v2;
    char c1[] = "con:";
    char c2[] = "con:";
    int r1 = 0; 
    r1 = vfs_open(c1,O_WRONLY,0644,&v1); 
    int r2 = 0;
    r2 = vfs_open(c2,O_WRONLY,0644,&v2); 
    oft[1].entry_vnode = v1;
    oft[2].entry_vnode = v2;
    if(r1 || r2){
        return r1 | r2;
    }
    return 0; 
} 

int sys_open(char *filename, int flags, int32_t *retval){
    struct vnode *vn;
    char *path;
    int err;
    size_t *path_len;
    int fd_temp = -1;

    path = kmalloc(PATH_MAX);
    path_len = kmalloc(sizeof(int));
    /* Copy the string from userspace to kernel space and check for valid address */
    err = copyinstr((const_userptr_t) filename, path, PATH_MAX, path_len);
    if(err){
        kfree(path);
        kfree(path_len);
        return err;
    }
    for(int i = 0; i < OPEN_MAX; i++){
        if(oft[i].entry_vnode == NULL){
            fd_temp = i;
            break;
        }
        return ENFILE;    
    }

    err = vfs_open(path, flags, 0, &vn);
    kfree(path);
    kfree(path_len);
    if(err){
        return err;
    }

    // store a reference of vnode
    oft[fd_temp].entry_vnode = vn;
    
    // set append
    if(flags & O_APPEND){
        struct stat info;
        off_t offset;
        offset = VOP_STAT(oft[fd_temp].entry_vnode, &info);
        if(offset)
            return offset;
        oft[fd_temp].entry_offset = info.st_size;
    }
    
    // update fdt
    curproc->fdt[fd_temp] = &(oft[fd_temp]);

    *retval = fd_temp;

    return 0;
}



