require 'mkmf'

def add_define(name)
  $defs.push("-D#{name}")
end

dir_config('aio')

have_func('aio_read', 'aio.h')

add_define 'RUBY18' if have_var('rb_trap_immediate', ['ruby.h', 'rubysig.h'])

$CFLAGS << ' -lrt'

create_makefile('aio')