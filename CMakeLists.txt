cmake_minimum_required(VERSION 2.6)

include_directories("${PROJECT_SOURCE_DIR}")

add_library(tinyrl tinyrl/tinyrl.c tinyrl/vt100/vt100.c
	tinyrl/history/history.c tinyrl/history/history_entry.c
	lub/string/string_dup.c lub/string/string_dupn.c lub/string/string_free.c
	lub/string/string_cat.c lub/string/string_catn.c
	lub/string/string_nocasecmp.c lub/string/string_nocasestr.c
	lub/ctype/ctype_tolower.c)

add_executable(example example.c)
target_link_libraries(example tinyrl)
