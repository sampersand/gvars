require 'mkmf'

$CFLAGS << ' -g -fsanitize=undefined'

create_makefile 'gvars/gvars'
