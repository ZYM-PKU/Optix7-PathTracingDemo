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


#include "SampleRenderer.h"

// our helper library for window handling
#include "glfWindow/GLFWindow.h"
#include <GL/gl.h>
#include "../common/imgui-1.87/imgui.h"
#include "../common/imgui-1.87/backends/imgui_impl_glfw.h"
#include "../common/imgui-1.87/backends/imgui_impl_opengl3.h"

/*! \namespace opz - Optix ZYM-PKU */
namespace opz {

  struct SampleWindow : public GLFCameraWindow
  {
    SampleWindow(const std::string &title,
                 const Model *model,
                 const Camera &camera,
                 const QuadLight &light,
                 const float worldScale)
      : GLFCameraWindow(title,camera.from,camera.at,camera.up,worldScale),
        sample(model,light)
    {
      sample.setCamera(camera);
      ImGui::CreateContext();     // Setup Dear ImGui context
      ImGui::StyleColorsDark();       // Setup Dear ImGui style
      ImGui_ImplGlfw_InitForOpenGL(handle, true);     // Setup Platform/Renderer backends
      ImGui_ImplOpenGL3_Init("#version 330");
    }
    
    virtual void render() override
    {
      if (cameraFrame.modified) {
        sample.setCamera(Camera{ cameraFrame.get_from(),
                                 cameraFrame.get_at(),
                                 cameraFrame.get_up() });
        cameraFrame.modified = false;
      }
      sample.render();
    }
    
    virtual void draw() override
    {
      sample.downloadPixels(pixels.data());
      if (fbTexture == 0)
        glGenTextures(1, &fbTexture);
      
      glBindTexture(GL_TEXTURE_2D, fbTexture);
      GLenum texFormat = GL_RGBA;
      GLenum texelType = GL_UNSIGNED_BYTE;
      glTexImage2D(GL_TEXTURE_2D, 0, texFormat, fbSize.x, fbSize.y, 0, GL_RGBA,
                   texelType, pixels.data());

      glDisable(GL_LIGHTING);
      glColor3f(1, 1, 1);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, fbTexture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      
      glDisable(GL_DEPTH_TEST);

      glViewport(0, 0, fbSize.x, fbSize.y);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0.f, (float)fbSize.x, 0.f, (float)fbSize.y, -1.f, 1.f);

  


      glBegin(GL_QUADS);
      {
        glTexCoord2f(0.f, 0.f);
        glVertex3f(0.f, 0.f, 0.f);
      
        glTexCoord2f(0.f, 1.f);
        glVertex3f(0.f, (float)fbSize.y, 0.f);
      
        glTexCoord2f(1.f, 1.f);
        glVertex3f((float)fbSize.x, (float)fbSize.y, 0.f);
      
        glTexCoord2f(1.f, 0.f);
        glVertex3f((float)fbSize.x, 0.f, 0.f);
      }
      glEnd();



      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      {
          ImGui::Begin("Main Panel");

          ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
          if (cameraFrameManip == flyModeManip)
              ImGui::Text("Current Mode:  Fly Mode");
          else if(cameraFrameManip == inspectModeManip)
              ImGui::Text("Current Mode:  Inspect Mode");

          ImGui::End();
      }

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    }
    
    virtual void resize(const vec2i &newSize) 
    {
      fbSize = newSize;
      sample.resize(newSize);
      pixels.resize(newSize.x*newSize.y);
    }

    virtual void keyAction(int key, int action, int mods)
    {

      if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
          exit(0);
          //glfwSetWindowShouldClose(handle, GLFW_TRUE);

       //相机控制
      if (key == GLFW_KEY_LEFT_ALT && action != GLFW_REPEAT) {   //按住左Alt键使用鼠标进行相机控制
          const bool pressed = (action == GLFW_PRESS);
          isPressed.Altpressed = pressed;
          lastMousePos = getMousePos();
      }
      if ((key == 'W' || key == 'w') && action !=  GLFW_REPEAT) {
          const bool pressed = (action == GLFW_PRESS);
          isPressed.Wpressed = pressed;
          lastMousePos = getMousePos();
      }
      if ((key == 'S' || key == 's') && action != GLFW_REPEAT) {
          const bool pressed = (action == GLFW_PRESS);
          isPressed.Spressed = pressed;
          lastMousePos = getMousePos();
      }
      if ((key == 'A' || key == 'a') && action != GLFW_REPEAT) {
          const bool pressed = (action == GLFW_PRESS);
          isPressed.Apressed = pressed;
          lastMousePos = getMousePos();
      }
      if ((key == 'D' || key == 'd') && action != GLFW_REPEAT) {
          const bool pressed = (action == GLFW_PRESS);
          isPressed.Dpressed = pressed;
          lastMousePos = getMousePos();
      }

      //键盘选项

      if ((key == 'N' || key == ' ' || key == 'n') && action == GLFW_PRESS) {
        sample.denoiserOn = !sample.denoiserOn;
        std::cout << "denoising now " << (sample.denoiserOn?"ON":"OFF") << std::endl;
      }
      if ((key == 'R' || key == 'r') && action == GLFW_PRESS) {
        sample.accumulate = !sample.accumulate;
        std::cout << "accumulation/progressive refinement now " << (sample.accumulate?"ON":"OFF") << std::endl;
      }
      if (key == ',' && action == GLFW_PRESS) {
        sample.launchParams.numPixelSamples
          = std::max(1,sample.launchParams.numPixelSamples-1);
        std::cout << "num samples/pixel now "
                  << sample.launchParams.numPixelSamples << std::endl;
      }
      if (key == '.' && action == GLFW_PRESS) {
        sample.launchParams.numPixelSamples
          = std::max(1,sample.launchParams.numPixelSamples+1);
        std::cout << "num samples/pixel now "
                  << sample.launchParams.numPixelSamples << std::endl;
      }

      if ((key == 'F' || key == 'f') && action == GLFW_PRESS) {
          std::cout << "Entering 'fly' mode" << std::endl;
          if (flyModeManip) cameraFrameManip = flyModeManip;//进入飞行模式
      }

      if ((key == 'I' || key == 'i') && action == GLFW_PRESS) {
          std::cout << "Entering 'inspect' mode" << std::endl;
          cameraFrameManip->reset();
          if (inspectModeManip) cameraFrameManip = inspectModeManip;//进入观察者模式
      }
      
    }
    

    vec2i                 fbSize;
    GLuint                fbTexture {0};
    SampleRenderer        sample;
    std::vector<uint32_t> pixels;
  };
  
  
  /*! main entry point to this example - initially optix, print hello
    world, then exit */
  extern "C" int main(int ac, char **av)
  {
    try {
      Model *model = loadOBJ(
#ifdef _WIN32
      // on windows, visual studio creates _two_ levels of build dir
      // (x86/Release)
      "../../models/f1car/mcl35m_2.obj"
#else
      // on linux, common practice is to have ONE level of build dir
      // (say, <project>/build/)...
      "../models/sponza.obj"
#endif
                             );
      Camera camera = { /*from*/vec3f(-5.f,0.f,5.f),
          /* at */model->bounds.center(),
          /* up */vec3f(0.f,1.f,0.f) };

      // some simple, hard-coded light ... obviously, only works for sponza
      const float light_size = 200.f;
      QuadLight light = { /* origin */ vec3f(-1000-light_size,800,-light_size),
                          /* edge 1 */ vec3f(2.f*light_size,0,0),
                          /* edge 2 */ vec3f(0,0,2.f*light_size),
                          /* power */  vec3f(3000000.f) };
                      
      // something approximating the scale of the world, so the
      // camera knows how much to move for any given user interaction:
      const float worldScale = length(model->bounds.span());

      SampleWindow *window = new SampleWindow("Optix 7 Project",
                                              model,camera,light,worldScale);
      window->enableFlyMode();
      
      std::cout << "Press 'r' to enable/disable accumulation/progressive refinement" << std::endl;
      std::cout << "Press 'n' to enable/disable denoising" << std::endl;
      std::cout << "Press ',' to reduce the number of paths/pixel" << std::endl;
      std::cout << "Press '.' to increase the number of paths/pixel" << std::endl;
      window->run();
      
    } catch (std::runtime_error& e) {
      std::cout << GDT_TERMINAL_RED << "FATAL ERROR: " << e.what()
                << GDT_TERMINAL_DEFAULT << std::endl;
	  std::cout << "Did you forget to copy sponza.obj and sponza.mtl into your optix7course/models directory?" << std::endl;
	  exit(1);
    }
    return 0;
  }
  
} // ::osc
