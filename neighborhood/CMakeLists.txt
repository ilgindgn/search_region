add_executable(neighborhood search_zone.c neighborhood.c)

target_link_libraries(neighborhood common)
target_compile_options(neighborhood PUBLIC -Wall -Wextra -std=gnu99)

install (TARGETS neighborhood DESTINATION bin)
