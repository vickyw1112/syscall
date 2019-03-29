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
    
    


}

 


