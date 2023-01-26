
PROJECT_TARGET_ADD(fedid-types)

	# compile any C file starting with a lower case
	file(GLOB CSOURCES [a-z]*.c)
    file(GLOB CHEADERS [a-z]*.h)

	# Define project Targets
	add_library(${TARGET_NAME} SHARED ${CSOURCES})

	# compilation include path 
	target_include_directories (${TARGET_NAME}
	    INTERFACE "${CMAKE_SOURCE_DIR}"
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
	install(TARGETS ${TARGET_NAME})

include(GNUInstallDirs)
configure_file(${TARGET_NAME}.pc.in ${TARGET_NAME}.pc @ONLY)

INSTALL(FILES
		${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.pc
		DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/pkgconfig
	)
INSTALL(FILES ${CHEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

