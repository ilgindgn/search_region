set (common_FILES
  macros.h
	typedefs.h
	clist.c
	clist.h
	dlist.c
	dlist.h
	list.c
	list.h
	dominance.c
	dominance.h
	input.c
	input.h
	output.c
	output.h
	stats.c
	stats.h
)

add_library(common ${common_FILES})
target_compile_options(common PUBLIC -Wall -Wextra -std=gnu99)
