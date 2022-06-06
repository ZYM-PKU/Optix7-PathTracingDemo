# ======================================================================== #
# Copyright 2018-2019 Ingo Wald                                            #
#                                                                          #
# Licensed under the Apache License, Version 2.0 (the "License");          #
# you may not use this file except in compliance with the License.         #
# You may obtain a copy of the License at                                  #
#                                                                          #
#     http://www.apache.org/licenses/LICENSE-2.0                           #
#                                                                          #
# Unless required by applicable law or agreed to in writing, software      #
# distributed under the License is distributed on an "AS IS" BASIS,        #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
# See the License for the specific language governing permissions and      #
# limitations under the License.                                           #
# ======================================================================== #


# ------------------------------------------------------------------
# import imgui submodule
# ------------------------------------------------------------------
include_directories(
  ${PROJECT_SOURCE_DIR}/common/imgui-1.87/
  ${PROJECT_SOURCE_DIR}/common/imgui-1.87/backends
)

# set common source
file(GLOB SRC
    ${PROJECT_SOURCE_DIR}/common/imgui-1.87/*.h
    ${PROJECT_SOURCE_DIR}/common/imgui-1.87/*.cpp
)

# set specific source and other option for platform
file (GLOB PLATFORM_SRC
    ${PROJECT_SOURCE_DIR}/common/imgui-1.87/backends/imgui_impl_glfw.*
    ${PROJECT_SOURCE_DIR}/common/imgui-1.87/backends/imgui_impl_opengl3.*
)


add_library(glfWindow
  GLFWindow.h
  GLFWindow.cpp
  ${SRC} ${PLATFORM_SRC}
  )