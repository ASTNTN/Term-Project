set(LIBRARY_NAME "udp-detector-client")

add_executable(${LIBRARY_NAME}
	client.c
)

target_include_directories(${LIBRARY_NAME} PUBLIC
	${CMAKE_SOURCE_DIR}/include
	/usr/include/libnl3
)

target_link_libraries(${LIBRARY_NAME} PUBLIC
	nl-3
	nl-genl-3
)

target_compile_options(${LIBRARY_NAME} PRIVATE
)
