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


int sys_open(char *filename, int flags, mode_t mode, int32_t *retval);

int sys_close(int fd);


#endif /* _FILE_H_ */
