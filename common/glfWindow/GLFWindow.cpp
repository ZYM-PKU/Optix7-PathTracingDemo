// ======================================================================== //
// Copyright 2022-2023 ZYM-PKU                                           //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "GLFWindow.h"
#include "../imgui-1.87/imgui.h"
#include "../imgui-1.87/backends/imgui_impl_glfw.h"
#include "../imgui-1.87/backends/imgui_impl_opengl3.h"

/*! \namespace opz - Optix ZYM-PKU */
namespace opz {
  using namespace gdt;
  
  static void glfw_error_callback(int error, const char* description)
  {
    fprintf(stderr, "Error: %s\n", description);
  }
  
  GLFWindow::~GLFWindow()
  {
    glfwDestroyWindow(handle);
    glfwTerminate();
  }

  GLFWindow::GLFWindow(const std::string &title)
  {
    glfwSetErrorCallback(glfw_error_callback);
    // glfwInitHint(GLFW_COCOA_MENUBAR, GLFW_FALSE);
      
    if (!glfwInit())
      exit(EXIT_FAILURE);
      
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
      
    handle = glfwCreateWindow(1200, 800, title.c_str(), NULL, NULL);
    if (!handle) {
      glfwTerminate();
      exit(EXIT_FAILURE);
    }
      
    glfwSetWindowUserPointer(handle, this);
    glfwMakeContextCurrent(handle);
    glfwSwapInterval( 1 );

  }

  /*! callback for a window resizing event */
  static void glfwindow_reshape_cb(GLFWwindow* window, int width, int height )
  {
    GLFWindow *gw = static_cast<GLFWindow*>(glfwGetWindowUserPointer(window));
    assert(gw);
    gw->resize(vec2i(width,height));
  }

  /*! callback for a key press */
  static void glfwindow_key_cb(GLFWwindow *window, int key, int scancode, int action, int mods) 
  {
    GLFWindow *gw = static_cast<GLFWindow*>(glfwGetWindowUserPointer(window));
    assert(gw);
    gw->keyAction(key, action, mods);
  }

  /*! callback for mouse scroll */
  static void glfwindow_scroll_cb(GLFWwindow* window, double xoffset, double yoffset)
  {
      GLFWindow* gw = static_cast<GLFWindow*>(glfwGetWindowUserPointer(window));
      assert(gw);
      gw->scrollAction(yoffset);
  }


  void GLFWindow::run()
  {
    int width, height;
    glfwGetFramebufferSize(handle, &width, &height);
    resize(vec2i(width,height));

    // glfwSetWindowUserPointer(window, GLFWindow::current);
    glfwSetFramebufferSizeCallback(handle, glfwindow_reshape_cb);
    glfwSetKeyCallback(handle, glfwindow_key_cb);
    glfwSetScrollCallback(handle, glfwindow_scroll_cb);
    //glfwSetMouseButtonCallback(handle, glfwindow_mouseButton_cb);
    //glfwSetCursorPosCallback(handle, glfwindow_mouseMotion_cb);
    
    while (!glfwWindowShouldClose(handle)) {
      render();
      draw();
        
      CameraMove();
      glfwSwapBuffers(handle);
      glfwPollEvents();
    }
  }

  // GLFWindow *GLFWindow::current = nullptr;
  
} // ::osc
