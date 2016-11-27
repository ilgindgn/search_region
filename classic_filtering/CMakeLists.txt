add_executable(classic_filtering search_zone.c classic_filtering.c)

#add_definitions (-DSZ_GP)
target_link_libraries(classic_filtering common)
target_compile_options(classic_filtering PUBLIC -Wall -Wextra -std=gnu99)

install (TARGETS classic_filtering DESTINATION bin)
