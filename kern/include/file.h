/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#include "types.h"

/* open file table entry defination*/
struct oft_entry{
    struct vnode *entry_vnode;
    int entry_refcount; // pointed by how many fd
    off_t entry_offset;
    size_t fd_num;
    struct lock *entry_lock;
};

/*
 * Put your function declarations and data types here ...
 */



int init_stdouterr(void);
int init_open_file_table(void);
void destroy_open_file_table(void);
int sys_open(char *filename, int flags, int32_t *retval);


#endif /* _FILE_H_ */
