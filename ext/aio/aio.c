#include "ruby.h"
#include "rubyio.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#ifdef _POSIX_ASYNCHRONOUS_IO
#include <aio.h>
#endif

#ifndef RSTRING_PTR
#define RSTRING_PTR(obj) RSTRING(obj)->ptr
#endif
 
#ifndef RSTRING_LEN
#define RSTRING_LEN(obj) RSTRING(obj)->len
#endif

#ifdef HAVE_RBTRAP
  #include <rubysig.h>
#else
  #define TRAP_BEG
  #define TRAP_END
#endif

typedef struct aiocb aiocb_t;

static VALUE rb_aio_read( aiocb_t *cb ){
	int err, ret;
	TRAP_BEG;
    ret = aio_read( cb );
	TRAP_END;
	if (ret < 0) rb_raise( rb_eIOError, "read schedule failure" );
    
	while ( err = aio_error( cb ) == EINPROGRESS );
	
	if ((ret = aio_return( cb )) > 0) {
		return rb_tainted_str_new((*cb).aio_buf, ret);
    } else {
		return INT2NUM(ret);
    }
}

static VALUE rb_aio_s_read( int argc, VALUE* argv, VALUE aio ){
	OpenFile *fptr;
	VALUE fname, length, offset, io;
	int fd, err, ret; 
    struct stat stats;
    aiocb_t cb;

    rb_scan_args(argc, argv, "03", &fname, &length, &offset);
    Check_Type(fname, T_STRING);
	io = rb_file_open(RSTRING_PTR(fname), "r");
	GetOpenFile(io, fptr);
	rb_io_check_readable(fptr); 	
    
    if (offset == Qnil)
      offset = INT2FIX(0);
    if (length == Qnil){
      fstat(fd, &stats);
      length = INT2FIX(stats.st_size);
    }

	(void) memset(&cb, 0, sizeof(struct aiocb));
	
    cb.aio_buf = malloc(NUM2INT(length) + 1);
	if (!cb.aio_buf) rb_raise( rb_eIOError, "not able to allocate a read buffer" );
	
	cb.aio_fildes = fileno(fptr->f);
	cb.aio_nbytes = NUM2INT(length);
	cb.aio_offset = NUM2INT(offset);
	cb.aio_sigevent.sigev_notify = SIGEV_NONE;
	cb.aio_sigevent.sigev_signo = 0;
	cb.aio_reqprio = 0;
	
	return rb_ensure( rb_aio_read, &cb, rb_io_close, io );
}	 

void Init_aio()
{

    rb_define_module_function( rb_cIO, "aio_read", rb_aio_s_read, -1 );

}