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


int sys_open(char *filename, int flags, int32_t *retval){
    struct vnode *vn;
    char *path;
    int err;
    size_t *path_len;

    path = kmalloc(PATH_MAX);
    path_len = kmalloc(sizeof(int));
    /* Copy the string from userspace to kernel space and check for valid address */
    err = copyinstr((const_userptr_t) filename, path, PATH_MAX, path_len);
    if(err){
        kfree(path);
        kfree(path_len);
        return err;
    }

    err = vfs_open(path, flags, 0, &vn);
    kfree(path);
    kfree(path_len);
    if(err){
        return err;
    }

    // get current fd table
    struct fd_table *fd_t = curproc->fd_t;

    // lock open file table for current proc
    lock_acquire(of_t->oft_lock);
    int fd_temp = -1;
    int of_index = -1;

    // find available spot in file descerptor in cur process fdt 
    for(int i = 0; i < OPEN_MAX; i++){
        if(fd_t->fd_entries[i] == FILE_CLOSED){
            fd_temp = i;
            break;
        }
    }
    // find available spot in open file table 
    for (int i = 0; i < OPEN_MAX; i++){
		if (of_t->openfiles[i] == NULL){
			of_index = i;
			break;
		}
	}
    
    if(of_index == -1 || fd_temp == -1){
        vfs_close(vn);
        lock_release(of_t->oft_lock);
		return EMFILE;
    }

    // create new file record
	struct open_file *of_entry = kmalloc(sizeof(struct open_file));
	if (of_entry == NULL) {
		vfs_close(vn);
		lock_release(of_t->oft_lock);
		return ENOMEM;
	}

    /* update fd table
    fd_t: 
    |a|b|c|d|e|
     0 1 2 3 4  <--index 
    of_t:
    |f1|f2|f3|f4|f5|
     a  b  c  d  e  <--index 
    */
	fd_t->fd_entries[fd_temp] = of_index;
	
	of_entry->vnode = vn;
	of_entry->refcount = 1;	
	of_entry->offset = 0;

    // update global open file table
	of_t->openfiles[of_index] = of_entry;
    
    // release lock for open file table
	lock_release(of_t->oft_lock);

	*retval = fd_temp;
    return 0;
}


/* this function is called when process run */
int file_table_init(void){
    int i, fd;

    /* if there no open file table then create open file table */
	if (of_t == NULL){
		of_t = kmalloc(sizeof(struct file_table));
		if (of_t == NULL){
			return ENOMEM;
		}

		/* create lock for the open file table */
		struct lock *oft_lock = lock_create("oft_lock");
		if (oft_lock == NULL){
			return ENOMEM;
		}
		of_t->oft_lock = oft_lock;

		/* empty the table */
		for (i = 0; i < OPEN_MAX; i++){
			of_t->openfiles[i] = NULL;
		}
	}

    /* create file descriptor table for each process */
    curproc->fd_t = kmalloc(sizeof(struct fd_table));
    if(curproc->fd_t == NULL){
        return ENOMEM;
    }

    /* empty the new table */
    for(fd = 0; fd < OPEN_MAX; fd++){
        curproc->fd_t->fd_entries[fd] = FILE_CLOSED;
    }
	return 0;
}

/* destroy file table for cur process*/
void file_table_destroy(void){
}


