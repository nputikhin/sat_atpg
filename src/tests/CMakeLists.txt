set(SOURCES
	test_main.cpp
	test_circuit_graph.cpp
	test_cnf.cpp
	test_fault_cnf.cpp
	test_fault_manager.cpp
	circuits.h
)

add_executable(tests ${SOURCES})

target_link_libraries(tests catch2 atpg_backend)

add_custom_command(
	TARGET tests
	COMMENT "Run tests"
	POST_BUILD
	COMMAND tests
)
