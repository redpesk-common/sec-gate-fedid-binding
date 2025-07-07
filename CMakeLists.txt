###########################################################################
# Copyright 2015-2025 IoT.bzh
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


cmake_minimum_required(VERSION 3.16)

project(
	fedid
	VERSION 1.1.0
	DESCRIPTION "Bind federated social identity with a local user identity"
	HOMEPAGE_URL "https://github.com/iotbzh/sec-gate-fedid-binding"
	LANGUAGES C
)


include(FindPkgConfig)
include(GNUInstallDirs)

set(AFM_APP_DIR ${CMAKE_INSTALL_PREFIX}/redpesk CACHE PATH "Applications directory")

pkg_check_modules(mod0 REQUIRED
	json-c
	afb-binding
	librp-utils-json-c)

include_directories(${mod0_INCLUDE_DIRS})
add_compile_options(${mod0_CFLAGS})
add_link_options(${mod0_LDFLAGS})
link_libraries(${mod0_LIBRARIES})

add_subdirectory(src)
