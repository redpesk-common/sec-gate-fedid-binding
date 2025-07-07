###########################################################################
# Copyright 201-2025 IoT.bzh
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

add_subdirectory(fedid-types)

# Define project Targets
add_library(
	sec-gate-fedid-binding
	SHARED
		fedid-binding.c
		sqldb-glue/sqldb-glue.c
)

# Binder exposes a unique public entry point
set_target_properties(
	sec-gate-fedid-binding
	PROPERTIES
		PREFIX ""
)

pkg_check_modules(sq3 REQUIRED sqlite3)

target_include_directories(sec-gate-fedid-binding PRIVATE ${sq3_INCLUDE_DIRS})
target_compile_options(sec-gate-fedid-binding PRIVATE ${sq3_CFLAGS})
target_link_options(sec-gate-fedid-binding PRIVATE ${sq3_LDFLAGS})
target_link_libraries(sec-gate-fedid-binding fedid-types ${sq3_LIBRARIES})

set(APPDIR ${AFM_APP_DIR}/sec-gate-fedid-binding)

install(TARGETS sec-gate-fedid-binding
	DESTINATION ${APPDIR}/lib)

configure_file(manifest.yml.in manifest.yml @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/manifest.yml
	DESTINATION ${APPDIR}/.rpconfig)

