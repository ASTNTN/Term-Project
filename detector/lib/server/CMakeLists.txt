set(LIBRARY_NAME "udp-detector-server")

add_library(${LIBRARY_NAME}
	server.c
)

target_include_directories(${LIBRARY_NAME} PUBLIC
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${LIBRARY_NAME} PUBLIC
	uring
)

target_compile_options(${LIBRARY_NAME} PRIVATE
)
