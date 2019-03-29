/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#include <types.h>

#define FILE_CLOSED -1

/* open file table entry defination*/
struct oft_entry{
    struct vnode *entry_vnode;
    int entry_refcount; // reference count of this open file
    off_t entry_offset;
    struct lock *entry_lock;
};

/* global open file table entry */
struct open_file{
	struct vnode *vnode;	/* the vnode this file represents */
	int refcount;			/* the reference count of this file */
	off_t offset;		/* read offset within the file */
};

/* global open file table */
struct file_table{
	struct lock *oft_lock;	/* open file table lock */
	struct open_file *openfiles[OPEN_MAX];	/* array of open files */
};

/* global open file table */
struct file_table *of_table;

/* init global open file table */
int open_file_table_init(void);

/* initiate fd_table for each proc */
int fd_table_init(void);

/* destroys the file table */
void file_table_destroy(void);


int sys_open(char *filename, int flags, mode_t mode, int32_t *retval);


int sys_write(int fd, const void *buf, size_t nbytes, int32_t *retval);

int sys_read(int fd, void *buf, size_t nbytes, int *retval);

#endif /* _FILE_H_ */
