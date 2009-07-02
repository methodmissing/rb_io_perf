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

#ifndef RARRAY_PTR
#define RARRAY_PTR(obj) RARRAY(obj)->ptr
#endif
 
#ifndef RARRAY_LEN
#define RARRAY_LEN(obj) RARRAY(obj)->len
#endif

#ifdef RUBY18
  #include "rubysig.h"
  #include "rubyio.h"
#else
  #include "ruby/io.h" 
  #define TRAP_BEG
  #define TRAP_END
#endif

#define AIO_MAX_LIST 16

typedef struct aiocb aiocb_t;

struct aio_read_multi_args {
	aiocb_t **list;
	int reads; 
};

static VALUE rb_aio_read( struct aio_read_multi_args *args ){
	int op, ret;
    VALUE results = rb_ary_new2( args->reads );
	
	TRAP_BEG;
    ret = lio_listio( LIO_WAIT, (*args).list, args->reads, NULL );
	TRAP_END;
	if (ret != 0) rb_raise( rb_eIOError, "read multi schedule failure" );

    for (op=0; op < args->reads; op++) {
		rb_ary_push( results, rb_tainted_str_new( (*args->list)[op].aio_buf, (*args->list)[op].aio_nbytes ) );
    }
	return results;
}

static void rb_io_closes( VALUE ios ){
  int io;

  for (io=0; io < RARRAY_LEN(ios); io++) {
	 rb_io_close( RARRAY_PTR(ios)[io] );
   } 
}

static VALUE rb_aio_s_read( VALUE aio, VALUE files ){
#ifdef RUBY18
	OpenFile *fptrs[AIO_MAX_LIST];
#else	
	rb_io_t *fptrs[AIO_MAX_LIST];
#endif
    struct aio_read_multi_args args;
	int fd, length, op;
    struct stat stats;
	aiocb_t cb[AIO_MAX_LIST];
	aiocb_t *list[AIO_MAX_LIST]; 
	
	int reads = RARRAY_LEN(files);
	VALUE ios = rb_ary_new2( reads );
	VALUE io;    

    bzero( (char *)list, sizeof(list) );
    for (op=0; op < reads; op++) {
      Check_Type(RARRAY_PTR(files)[op], T_STRING);
      io = rb_file_open(RSTRING_PTR(RARRAY_PTR(files)[op]), "r");
	  rb_ary_push( ios, io );
	  GetOpenFile(io, fptrs[op]);
	  rb_io_check_readable(fptrs[op]); 	

#ifdef RUBY18
  	  fd = fileno(fptrs[op]->f);
#else	
  	  fd = fptrs[op]->fd;
#endif
      fstat(fd, &stats);
      length = stats.st_size;

      bzero(&cb[op], sizeof(aiocb_t));

	  cb[op].aio_buf = malloc(length + 1);
 	  if (!cb[op].aio_buf) rb_raise( rb_eIOError, "not able to allocate a read buffer" );

  	  cb[op].aio_fildes = fd;
	  cb[op].aio_nbytes = length;
	  cb[op].aio_offset = 0;
	  cb[op].aio_sigevent.sigev_notify = SIGEV_NONE;
	  cb[op].aio_sigevent.sigev_signo = 0;
	  cb[op].aio_sigevent.sigev_value.sival_int = 0;
	  cb[op].aio_lio_opcode = LIO_READ;
      list[op] = &cb[op];
    }
	args.list = list;
	args.reads = reads;
    return rb_ensure( rb_aio_read, &args, rb_io_closes, ios );
}

void Init_aio()
{

    rb_define_module_function( rb_cIO, "aio_read", rb_aio_s_read, -2 );

}