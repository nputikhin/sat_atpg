add_library(sat_solver sat_solver.h sat_solver.cpp)

if(USE_CADICAL)
	add_definitions(-DUSE_CADICAL=1)

	target_link_libraries(sat_solver cadical_sat)

	add_library(cadical_sat cadical.h cadical.cpp)

	add_dependencies(cadical_sat cadical)
	target_link_libraries(cadical_sat cadical)
endif()
