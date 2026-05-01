set(EXECUTABLE_NAME "udp-detector-server")

add_executable(${EXECUTABLE_NAME}
	server.c
)

target_include_directories(${EXECUTABLE_NAME} PUBLIC
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
	uring
	atomic
)

target_compile_options(${EXECUTABLE_NAME} PRIVATE
)
