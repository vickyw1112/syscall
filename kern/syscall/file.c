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

    // check of_table
	struct file_descriptor * fdp = &(curproc->fd_table[fd]);
    if(of_table[fdp->index] == NULL) {
        *err = EBADF;
        return -1;
    }

	//Check the premission
	if(fdp->mode == O_WRONLY) {
        *err = EBADF;
        return -1;
    }

	struct opened_file * of = of_table[fdp->index];

	//for debug
	KASSERT(of != NULL);

	void * kbuf = kmalloc(sizeof(void *) * nbytes);
	if(kbuf == NULL) {
        *err = EFAULT;
        return -1;
    }

	struct iovec _iovec;
	struct uio _uio;
	int result;

	_iovec.iov_ubase = (userptr_t) buf;

    //lock offset and start reading
	lock_acquire(of->of_lock);

	uio_kinit(&_iovec, &_uio, kbuf, nbytes, of->of_offset, UIO_READ);

	result = VOP_READ(of->of_vnode, &_uio);

	if (result)
	{
		lock_release(of->of_lock);
		kfree(kbuf);
        *err = result;
		return -1;
	}

    // the length of read
    int len = nbytes - _uio.uio_resid;
    // copy the kernal buffer to userland buffer
    result = copyout(kbuf, buf, len);

	kfree(kbuf);
    if (result && len) {
        lock_release(of->of_lock);
        *err = result;
        return -1;
    }
    // update offset
	of->of_offset += len;
	lock_release(of->of_lock);

	return len;
}

 


