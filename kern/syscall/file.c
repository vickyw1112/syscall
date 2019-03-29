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


static int std_init(void){
    int fd;
    //connect to console std
    char c0[] = "con:";
    char c1[] = "con:";
    char c2[] = "con:";
    int r0 = sys_open(c0, O_RDONLY, 0, &fd);
    int r1 = sys_open(c1, O_WRONLY, 0, &fd);
    int r2 = sys_open(c2, O_WRONLY, 0, &fd);
    if(r0)
        return r0;
    if(r1)
        return r1;
    if(r2)
        return r2;
    return 0;
}


int sys_open(char *filename, int flags, mode_t mode, int32_t *retval){
    struct vnode *vn;
    int err;

    err = vfs_open(filename, flags, mode, &vn);
    if(err){
        return err;
    }

    // get current fd table
    int *fd_table = curproc->fd_table;

    // lock open file table for current proc
    lock_acquire(of_table->oft_lock);
    int fd_temp = -1;
    int of_index = -1;

    // find available spot in file descerptor in cur process fdt 
    for(int i = 0; i < OPEN_MAX; i++){
        if(fd_table[i] == FILE_CLOSED){
            fd_temp = i;
            break;
        }
    }
    // find available spot in open file table 
    for (int i = 0; i < OPEN_MAX; i++){
		if (of_table->openfiles[i] == NULL){
			of_index = i;
			break;
		}
	}
    
    if(of_index == -1 || fd_temp == -1){
        vfs_close(vn);
        lock_release(of_table->oft_lock);
		return EMFILE;
    }

    // create new file record
	struct open_file *of_entry = kmalloc(sizeof(struct open_file));
	if (of_entry == NULL) {
		vfs_close(vn);
		lock_release(of_table->oft_lock);
		return ENOMEM;
	}

    /* update fd table
    fd_t: 
    |a|b|c|d|e|
     0 1 2 3 4  <--index 
    of_table:
    |f1|f2|f3|f4|f5|
     a  b  c  d  e  <--index 
    */
	fd_table[fd_temp] = of_index;
	
	of_entry->vnode = vn;
	of_entry->refcount = 1;	
	of_entry->offset = 0;

    // update global open file table
	of_table->openfiles[of_index] = of_entry;
    
    // release lock for open file table
	lock_release(of_table->oft_lock);

	*retval = fd_temp;
    return 0;
}

int sys_write(int fd, userptr_t buf, size_t nbytes, int32_t nbytes){
    return 0;
}


/* this function is called when process run */
int fd_table_init(void){
    int fd;
    /* empty the new table */
    for(fd = 0; fd < OPEN_MAX; fd++){
        curproc->fd_table[fd] = FILE_CLOSED;
    }

    int err = std_init();
    if(err){
        return err;
    }
	return 0;
}

// TODO
/* destroy file table for cur process*/
void file_table_destroy(void){
}

/* init global open file table */
int open_file_table_init(void){
    int i;
    
    of_table = kmalloc(sizeof(struct file_table));
    if (of_table == NULL){
        return ENOMEM;
    }

    /* create lock for the open file table */
    struct lock *oft_lock = lock_create("oft_lock");
    if (oft_lock == NULL){
        return ENOMEM;
    }
    of_table->oft_lock = oft_lock;

    /* empty the table */
    for (i = 0; i < OPEN_MAX; i++){
        of_table->openfiles[i] = NULL;
    }
    return 0;
}