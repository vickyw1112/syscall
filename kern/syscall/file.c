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


int sys_close(int fd){
    if(fd < 0 || fd > OPEN_MAX)
        return EBADF;
    if(!lock_do_i_hold(of_table->oft_lock)){
        lock_acquire(of_table->oft_lock);
    }

    int of_index = curproc->fd_table[fd];
    if(of_index == FILE_CLOSED){
        lock_release(of_table->oft_lock);
        return EBADF;
    }
    struct open_file* open_file = of_table->openfiles[of_index];
    
    // cleaning fd_table 
    curproc->fd_table[fd] = FILE_CLOSED;
    
    if(open_file->refcount == 1){
        vfs_close(open_file->vnode);
        of_table->openfiles[of_index] = NULL;
    }else{
        open_file->refcount = open_file->refcount - 1;
    }
    
    lock_release(of_table->oft_lock);
    return 0;
}

//duplicates an old file handle to a new one 
//
int sys_dup2(int oldfd, int newfd, int *retval){
	
	if(oldfd < 0 || oldfd >= __OPEN_MAX ||
        newfd < 0 || newfd >= __OPEN_MAX){
        return EBADF;
    }
    
    if(oldfd==newfd){
    	*retval = newfd;
        return 0;
    }
    int old_of_index = curproc->fd_table[oldfd];
    if(old_of_index == FILE_CLOSED){
    	return EBADF; 
    }
    
    if(of_table->openfiles[oldfd]==NULL){
    	return EBADF;
    }
    
    //lock_acquire(of_table->oft_lock);
    
    curproc->fd_table[newfd] =curproc->fd_table[oldfd];
    
    of_table->openfiles[old_of_index]->refcount++;
    
    //lock_release(of_table->oft_lock);
    //*retval = 0;
    *retval = newfd;
    return 0;
}

int sys_write(int fd, const void *buf, size_t nbytes, int32_t *retval){
    if(fd < 0 || fd >= OPEN_MAX) {
        return EBADF;
    }
    

    struct vnode *vn;
    int of_index = curproc->fd_table[fd];
    if(of_index == FILE_CLOSED){
    	return EBADF; 
    }
    lock_acquire(of_table->oft_lock);
    vn = of_table->openfiles[of_index]->vnode; 
    
    struct iovec iovec; 
    struct uio uio; 
    
    
    uio_uinit(&iovec, &uio,(userptr_t)buf,nbytes,of_table->openfiles[of_index]->offset, UIO_WRITE);
    
    int err = VOP_WRITE(vn,&uio);
    
    if(err){
    	lock_release(of_table->oft_lock);
    	
    	return err; 
    }  
    lock_release(of_table->oft_lock);
    *retval = uio.uio_offset - of_table->openfiles[of_index]->offset;
    of_table->openfiles[of_index]->offset = uio.uio_offset;
    return 0;
     
}


int sys_read(int fd, const void *buf, size_t nbytes, int *retval){
	if(fd < 0 || fd >= OPEN_MAX) {
        return EBADF;
    }
    
    struct vnode *vn;
    int of_index = curproc->fd_table[fd];
    if(of_index == FILE_CLOSED){
    	return EBADF; 
    }
    lock_acquire(of_table->oft_lock);
    vn = of_table->openfiles[of_index]->vnode; 
    
    struct iovec iovec; 
    struct uio uio; 
    
    
    uio_uinit(&iovec, &uio,(userptr_t)buf,nbytes,of_table->openfiles[of_index]->offset, UIO_READ);
    
    int err = VOP_READ(vn,&uio);
    
    if(err){
    	lock_release(of_table->oft_lock);
    	return err; 
    }  
    //int len = nbytes - uio.uio_resid;
    //kprintf("length of file %d \n",len);
    lock_release(of_table->oft_lock);
    *retval = uio.uio_offset - of_table->openfiles[of_index]->offset;
    of_table->openfiles[of_index]->offset = uio.uio_offset;
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
