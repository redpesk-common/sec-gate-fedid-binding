###########################################################################
# Copyright 201 IoT.bzh
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################


# Add target to project dependency list
PROJECT_TARGET_ADD(sec-gate-fedid-binding)

	# compile any C file starting with a lower case
	file(GLOB CSOURCES [a-z]*.c)
	file(GLOB CHEADERS [a-z]*.h)

	# Define project Targets
	add_library(${TARGET_NAME} MODULE  ${CSOURCES})

	# Binder exposes a unique public entry point
	SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		PREFIX ""
		LABELS "LIBRARY"
		LINK_FLAGS "${BINDINGS_LINK_FLAG}"
		IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/*${TARGET_NAME}.so
		OUTPUT_NAME ${TARGET_NAME}
	)

	add_dependencies(${TARGET_NAME} sqldb-glue fedid-types)
    target_link_libraries(${TARGET_NAME} sqldb-glue fedid-types)
	target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sqldb-glue ${CMAKE_CURRENT_SOURCE_DIR}/fedid-types)

add_subdirectory(sqldb-glue)
add_subdirectory(fedid-types)

