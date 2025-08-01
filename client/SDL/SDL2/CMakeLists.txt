# FreeRDP: A Remote Desktop Protocol Implementation
# FreeRDP SDL Client
#
# Copyright 2022 Armin Novak <anovak@thincast.com>
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
set(MODULE_NAME "sdl2-freerdp")

find_package(SDL2 REQUIRED)

find_package(Threads REQUIRED)

add_subdirectory(dialogs)
set(SRCS
    sdl_types.hpp
    sdl_utils.cpp
    sdl_utils.hpp
    sdl_kbd.cpp
    sdl_kbd.hpp
    sdl_touch.cpp
    sdl_touch.hpp
    sdl_pointer.cpp
    sdl_pointer.hpp
    sdl_disp.cpp
    sdl_disp.hpp
    sdl_monitor.cpp
    sdl_monitor.hpp
    sdl_freerdp.hpp
    sdl_freerdp.cpp
    sdl_channels.hpp
    sdl_channels.cpp
    sdl_window.hpp
    sdl_window.cpp
)

list(
  APPEND
  LIBS
  winpr
  freerdp
  freerdp-client
  Threads::Threads
  sdl2_client_res
  sdl2-dialogs
  sdl-common-aad-view
  sdl-common-prefs
)

if(NOT WITH_SDL_LINK_SHARED)
  list(APPEND LIBS SDL2::SDL2-static)
  set_target_properties(SDL2::SDL2-static PROPERTIES SYSTEM TRUE)
else()
  list(APPEND LIBS SDL2::SDL2)
  set_target_properties(SDL2::SDL2 PROPERTIES SYSTEM TRUE)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

addtargetwithresourcefile(${MODULE_NAME} "${WIN32_GUI_FLAG}" "${PROJECT_VERSION}" SRCS)

target_link_libraries(${MODULE_NAME} PRIVATE ${LIBS})

set_property(TARGET ${MODULE_NAME} PROPERTY FOLDER "Client/SDL")

get_target_property(SDL_CLIENT_BINARY_NAME ${MODULE_NAME} OUTPUT_NAME)
if(NOT WITH_CLIENT_SDL_VERSIONED)
  string(REPLACE "${MODULE_NAME}" "${PROJECT_NAME}" SDL_CLIENT_BINARY_NAME "${SDL_CLIENT_BINARY_NAME}")
  set_target_properties(${MODULE_NAME} PROPERTIES OUTPUT_NAME ${SDL_CLIENT_BINARY_NAME})
endif()

string(TIMESTAMP SDL_CLIENT_YEAR "%Y")
set(SDL_CLIENT_UUID "com.freerdp.client.sdl2")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sdl_config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/sdl_config.hpp @ONLY)

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT client)
install_freerdp_desktop("${MODULE_NAME}" "${SDL_CLIENT_UUID}")

add_subdirectory(man)
