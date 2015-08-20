cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wmissing-prototypes -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

add_library(tinyrl tinyrl.c history.c complete.c utf8.c)

add_executable(example example.c)
target_link_libraries(example tinyrl)

add_custom_target(data DEPENDS utf8data.c)

add_custom_command(
	OUTPUT utf8data.c
	COMMAND python ${CMAKE_SOURCE_DIR}/utf8data.py > utf8data.c
	DEPENDS utf8data.py UnicodeData.txt EastAsianWidth.txt GraphemeBreakProperty.txt)

add_custom_command(
	OUTPUT UnicodeData.txt
	COMMAND curl -o UnicodeData.txt http://www.unicode.org/Public/UNIDATA/UnicodeData.txt)
add_custom_command(
	OUTPUT EastAsianWidth.txt
	COMMAND curl -o EastAsianWidth.txt http://www.unicode.org/Public/UNIDATA/EastAsianWidth.txt)
add_custom_command(
	OUTPUT GraphemeBreakProperty.txt
	COMMAND curl -o GraphemeBreakProperty.txt http://www.unicode.org/Public/UCD/latest/ucd/auxiliary/GraphemeBreakProperty.txt)

file(GLOB headers *.h)
install(FILES ${headers} DESTINATION include/tinyrl)
install(TARGETS tinyrl ARCHIVE DESTINATION lib)
