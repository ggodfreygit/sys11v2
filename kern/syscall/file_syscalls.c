/*
 * File-related system call implementations.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/seek.h>
#include <kern/stat.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <copyinout.h>
#include <vfs.h>
#include <vnode.h>
#include <openfile.h>
#include <filetable.h>
#include <syscall.h>

/*
 * open() - get the path with copyinstr, then use openfile_open and
 * filetable_place to do the real work.
 */
int
sys_open(const_userptr_t upath, int flags, mode_t mode, int *retval)
{
	const int allflags = O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_NOCTTY;
	if(!allflags)		//invalid flag detected!
		return allflags;
	
	char *kpath = (char*)kmalloc(sizeof(char)*PATH_MAX);
	struct openfile *file;
	int result = 0;
	
	result = copyinstr(upath, kpath, PATH_MAX, NULL);
	if(result) {	//copyinstr fails!
		return result;
	}
	result = openfile_open(kpath, flags, mode, &file);
	if(result){	//openfile fails!
		return result;
	}
	result = filetable_place(curproc->p_filetable, file, retval);
	if(result){	//filetable_place fails!
		return result;
	}
	
	/* 
	 * Your implementation of system call open starts here.  
	 *
	 * Check the design document design/filesyscall.txt for the steps
	 */
/*
	(void) upath; // suppress compilation warning until code gets written
	(void) flags; // suppress compilation warning until code gets written
	(void) mode; // suppress compilation warning until code gets written
	(void) retval; // suppress compilation warning until code gets written
	(void) allflags; // suppress compilation warning until code gets written
	(void) kpath; // suppress compilation warning until code gets written
	(void) file; // suppress compilation warning until code gets written
*/
	return result;

}

/*
 * read() - read data from a file
 */
int
sys_read(int fd, userptr_t buf, size_t size, int *retval)
{
       int result = 0;
	struct openfile *file;
	result = filetable_get(curproc->p_filetable, fd, &file);
	if(result){ //filetable_get fails!!
		return result;
}
	lock_acquire(file->of_offsetlock);		//lock the seek position of the file!
	if(file->of_accmode == O_WRONLY){		//file is write only! 
		lock_release(file->of_offsetlock);
		return EBADF; //return appropriate error code!
	}

	struct iovec io;
	struct uio data_uio;
	uio_kinit(&io, &data_uio, buf, size, file->of_offset, UIO_READ);	//construct uio!
	//CALL VOP_READ!
	result = VOP_READ(file->of_vnode, &data_uio);
	if(result){	//VOPREAD fails!!!
		lock_release(file->of_offsetlock);
		return result;
}
	file->of_offset = data_uio.uio_offset;		//update seek position
	lock_release(file->of_offsetlock);		//relinquish lock!
	
	filetable_put(curproc->p_filetable, fd, file);	//filetable_put!
	
	*retval = size - data_uio.uio_resid;		//set retval to the amount of data read!
	
       /* 
        * Your implementation of system call read starts here.  
        *
        * Check the design document design/filesyscall.txt for the steps
        */
/*
       (void) fd; // suppress compilation warning until code gets written
       (void) buf; // suppress compilation warning until code gets written
       (void) size; // suppress compilation warning until code gets written
       (void) retval; // suppress compilation warning until code gets written
*/
       return result;
}

/*
 * write() - write data to a file
 */
int
sys_write(int fd, userptr_t buf, size_t size, int *retval)
{
	int result = 0;
	struct openfile *file;
	result = filetable_get(curproc->p_filetable, fd, &file);
	if(result){ //filetable_get fails!!
		return result;
}
	lock_acquire(file->of_offsetlock);		//lock the seek position of the file!
	if(file->of_accmode == O_RDONLY){		//file is read only! 
		lock_release(file->of_offsetlock);
		return EBADF; //return appropriate error code!
	}

	struct iovec io;
	struct uio data_uio;
	uio_kinit(&io, &data_uio, buf, size, file->of_offset, UIO_WRITE);	//contruct uio!
	//CALL VOP_WRITE!
	result = VOP_WRITE(file->of_vnode, &data_uio);
	if(result){	//VOPWRITE fails!!!
		lock_release(file->of_offsetlock);
		return result;
}
	file->of_offset = data_uio.uio_offset;		//update seek position
	lock_release(file->of_offsetlock);		//relinquish lock!
	
	filetable_put(curproc->p_filetable, fd, file);	//filetable_put!
	
	*retval = size - data_uio.uio_resid;		//set retval to the amount of data written!
	
       /* 
        * Your implementation of system call read starts here.  
        *
        * Check the design document design/filesyscall.txt for the steps
        */
/*
       (void) fd; // suppress compilation warning until code gets written
       (void) buf; // suppress compilation warning until code gets written
       (void) size; // suppress compilation warning until code gets written
       (void) retval; // suppress compilation warning until code gets written
*/
       return result;
}
/*
 * close() - remove from the file table.
 */
int
sys_close(int fd)
{
	int result = 0;
	if(!filetable_okfd(curproc->p_filetable, fd)){		//fd number is not valid!
		return EBADF;	
}
	struct openfile *file;
	filetable_placeat(curproc->p_filetable, NULL, fd, &file);	//replace file entry at fd with NULL, get openfile that was there
	if(file != NULL){	//if the previous entry was not also NULL
		openfile_decref(file);	//decref the open file!
}
	




//(void) fd; // suppress compilation warning until code gets written

	return result;
}
/* 
* meld () - combine the content of two files word by word into a new file
*/
int sys_meld(const_userptr_t filename1, const_userptr_t filename2, const_userptr_t filename3, int flags, mode_t  mode, int *retval)
{
	int result = 0;
	char *kpath1 = (char*)kmalloc(sizeof(char)*PATH_MAX);
	char *kpath2 = (char*)kmalloc(sizeof(char)*PATH_MAX);
	char *kpath3 = (char*)kmalloc(sizeof(char)*PATH_MAX);

	result = copyinstr(filename1, kpath1, PATH_MAX, NULL);
	if(result) {	//copyinstr fails!
		return result;
	}
	
	result = copyinstr(filename2, kpath2, PATH_MAX, NULL);
	if(result) {	//copyinstr fails!
		return result;
	}

	result = copyinstr(filename3, kpath3, PATH_MAX, NULL);
	if(result) {	//copyinstr fails!
		return result;
	}
	struct openfile *file1;
	struct openfile *file2;
	struct openfile *file3;
	result = openfile_open(kpath1, flags, mode, &file1);
	if(result){	//openfile fails!
		return result;
	}
	result = openfile_open(kpath2, flags, mode, &file2);
	if(result){	//openfile fails!
		return result;
	}
	result = openfile_open(kpath3, flags, mode, &file3);
	if(result){	//openfile fails!
		return result;
	}
	result = filetable_place(curproc->p_filetable, file1, retval);
	int fd1 = *retval;
	if(result){	//filetable_place fails!
		return result;
	}
	result = filetable_place(curproc->p_filetable, file2, retval);
	int fd2 = *retval;
	if(result){	//filetable_place fails!
		return result;
	}
	result = filetable_place(curproc->p_filetable, file3, retval);
	if(result){	//filetable_place fails!
		return result;
	}
	int fd3 = *retval;
	//while there is still more file to be read in each of file1 and file2,
	//loop every four bytes and read 4 bytes from file2 into one buffer, 4 bytes from file1
	//into another buffer, merge the buffers, then sys_write to file 3....
	//struggling with the implementation on this.......
	
	//final retval will be amount of bytes written to file three, a value easily totalled from the loop
	sys_close(fd1);
	sys_close(fd2);
	sys_close(fd3);
	/*

	(void) filename1; // suppress compilation warning until code gets written
(void) filename2; // suppress compilation warning until code gets written
(void) filename3; // suppress compilation warning until code gets written
	(void) flags; // suppress compilation warning until code gets written
	(void) mode; // suppress compilation warning until code gets written
	(void) retval; // suppress compilation warning until code gets written
*/



	return result;

}
