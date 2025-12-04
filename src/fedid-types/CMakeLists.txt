
# Define project Targets
add_library(
	fedid-types
	SHARED
		fedid-types.c
		fedid-types-glue.c
)

install(TARGETS fedid-types)

configure_file(fedid-types.pc.in fedid-types.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fedid-types.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES fedid-types.h fedid-types-glue.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

