PROJECT_TARGET_ADD(sqldb-glue)

	# compile any C file starting with a lower case
	file(GLOB CSOURCES [a-z]*.c)

	# Define project Targets
	add_library(${TARGET_NAME} STATIC ${CSOURCES})

	# Alsa Plugin properties
	SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
			OUTPUT_NAME ${TARGET_NAME}
	)

	# compilation include path
	target_include_directories(${TARGET_NAME}
	    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../fedid-types
    )
