#include "ruby.h"
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

static VALUE mAio, eAio;

static VALUE rb_aio_read( int argc, VALUE* argv, VALUE aio ){
	VALUE fname, length, offset;
	int fd, ret; 
    struct stat stats;
    struct aiocb cb;

    rb_scan_args(argc, argv, "03", &fname, &length, &offset);
    Check_Type(fname, T_STRING);  
    FILE * file = fopen( RSTRING_PTR(fname), "r+" );
	fd = fileno(file);     

    if (offset == Qnil)
      offset = 0;
    if (length == Qnil)
      fstat(fd, &stats);
    length = stats.st_size;

    bzero( (char *)&cb, sizeof(struct aiocb) );
    cb.aio_buf = malloc(length+1);
	if (!cb.aio_buf) rb_raise( eAio, "not able to allocate a read buffer" );
	
	cb.aio_fildes = fd;
	cb.aio_nbytes = length;
	cb.aio_offset = offset;

	TRAP_BEG;
	ret = aio_read( &cb );
	TRAP_END;
	if (ret < 0) rb_raise( eAio, "read failure" );
	while ( aio_error( &cb ) == EINPROGRESS ) ;
	if ((ret = aio_return( &cb )) > 0) {
		return rb_tainted_str_new( cb.aio_buf, cb.aio_nbytes );
    } else {
		return INT2NUM(ret);
    }
}	 

void Init_aio()
{
    mAio = rb_define_module("AIO");

    eAio = rb_define_class_under(mAio, "Error", rb_eStandardError);

    rb_define_module_function( mAio, "read", rb_aio_read, -1 );

}