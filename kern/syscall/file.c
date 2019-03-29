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

/*
 * Add your file-related functions here ...
 */


// return readed bytes when success
// return val < 0 when fail
ssize_t sys_read(int fd, void *buf, size_t nbytes, int* err) {
	// handle bad reference
	if (fd < 0 || fd >= OPEN_MAX) {
        *err = EBADF;
        return -1;
    }

    // check buffer
    if (buf == NULL) {
        *err = EFAULT;
        return -1;
    }
    if(){
    	*err = EBADF;
    	return -1;
    }
    
    


}

int sys_write(int fd, const void *buf, size_t nbytes, int32_t *retval){
	if(fd < 0 || fd >= OPEN_MAX) {
        return EBADF;
    }
    

    struct vnode *vn;
    int of_index = curproc->fd_table[fd];
    if(of_index = FILE_CLOSED){
    	return EBADF; 
    }
    lock_acquire(of_table->oft_lock);
    vn = of_table[of_index]->vnode; 
    
    struct iovec iovec; 
    struct uio uio; 
    
    
    uio_uinit(&iovec, &uio,		(userptr_t)buf,nbytes,,of_table[of_index]->offset, UIO_WRITE);
    
    int err = VOP_WRITE(&vn,&uio);
    
    if(err){
    	lock_release(of_table->oft_lock);
    	
    	return err; 
    }  
    lock_release(of_table->oft_lock);
    *retval = 1;
    return 0;
    
 
}


