
PROJECT_TARGET_ADD(fedid-types)

	# compile any C file starting with a lower case
	file(GLOB CSOURCES [a-z]*.c)

	# Define project Targets
	add_library(${TARGET_NAME} SHARED ${CSOURCES})

	# Alsa Plugin properties
	SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
			LABELS "BINDING"
			PREFIX ""
			SUFFIX ".so"
			OUTPUT_NAME ${TARGET_NAME}
	)

	# compilation include path 
	target_include_directories (${TARGET_NAME}
	    INTERFACE "${CMAKE_SOURCE_DIR}"
    )
