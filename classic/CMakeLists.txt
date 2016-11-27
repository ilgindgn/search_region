add_executable(classic search_zone.c classic.c)

#add_definitions (-DSZ_GP -DSZ_ENH_UPD_NO_MALLOC)
add_definitions (-DSZ_ENH_UPD_NO_MALLOC)
target_link_libraries(classic common)
target_compile_options(classic PUBLIC -Wall -Wextra -std=gnu99)

install (TARGETS classic DESTINATION bin)
