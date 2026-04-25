set(EXECUTABLE_NAME "udp-detector")

add_executable(${EXECUTABLE_NAME}
	main.c
)

target_include_directories(${EXECUTABLE_NAME} PUBLIC
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
	udp-detector-client
	udp-detector-server
)

target_compile_options(${EXECUTABLE_NAME} PRIVATE
)
