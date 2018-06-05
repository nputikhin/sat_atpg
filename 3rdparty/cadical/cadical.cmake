include(ExternalProject)

SET(CADICAL_PREFIX cadical06w)
SET(CADICAL_URL https://github.com/arminbiere/cadical/archive/rel-06w.zip)

ExternalProject_Add(${CADICAL_PREFIX}
	PREFIX ${CADICAL_PREFIX}
	URL ${CADICAL_URL}
	CONFIGURE_COMMAND ./configure
	BUILD_IN_SOURCE 1
	INSTALL_COMMAND ""
)


ExternalProject_Get_Property(${CADICAL_PREFIX} SOURCE_DIR)

add_library(cadical STATIC IMPORTED)
add_dependencies(cadical ${CADICAL_PREFIX})

file(MAKE_DIRECTORY ${SOURCE_DIR}/src)

set_property(TARGET cadical PROPERTY IMPORTED_LOCATION ${SOURCE_DIR}/build/libcadical.a)
set_property(TARGET cadical PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/src)


