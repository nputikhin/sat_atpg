set(BACKEND_SOURCES
	circuit_graph.h
	circuit_graph.cpp
	iscas89_parser.h
	iscas89_parser.cpp
	circuit_to_cnf.h
	circuit_to_cnf.cpp
	fault_cnf.h
	fault_cnf.cpp
	fault_manager.h
	fault_manager.cpp
	cnf.h
	cnf.cpp
	solver_proxy.h
	object_set.h
	util/log.h
	util/log.cpp
	util/timer.h
)

add_library(atpg_backend ${BACKEND_SOURCES})
target_link_libraries(atpg_backend sat_solver)
target_include_directories(atpg_backend PRIVATE util)

set(SOURCES
	main.cpp
)

add_subdirectory(sat)

add_executable(atpgSat main.cpp)

target_link_libraries(atpgSat atpg_backend)

add_subdirectory(tests)
