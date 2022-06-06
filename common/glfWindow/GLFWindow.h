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


#pragma once

// common gdt helper tools
#include "gdt/math/AffineSpace.h"
// glfw framework
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

/*! \namespace opz - Optix ZYM-PKU */
namespace opz {
  using namespace gdt;

  struct CameraFrame {
    CameraFrame(const float worldScale): motionSpeed(worldScale) {}
    
    vec3f getPOI() const
    { 
      return position - poiDistance * frame.vz;
    }
      
    /*! re-compute all orientation related fields from given
      'user-style' camera parameters */
    void setOrientation(/* camera origin    : */const vec3f &origin,
                        /* point of interest: */const vec3f &interest,
                        /* up-vector        : */const vec3f &up)
    {
      position = origin;
      upVector = up;
      frame.vz
        = (interest==origin)
        ? vec3f(0,0,1)
        : /* negative because we use NEGATIZE z axis */ - normalize(interest - origin);
      frame.vx = cross(up,frame.vz);
      if (dot(frame.vx,frame.vx) < 1e-8f)
        frame.vx = vec3f(0,1,0);
      else
        frame.vx = normalize(frame.vx);

      frame.vy = normalize(cross(frame.vz,frame.vx));

      poiDistance = length(interest-origin);
      forceUpFrame();
      origin_state = { origin, interest, up };

    }


    //reset the camera state
    void resetOrientation() {

        vec3f origin = origin_state.origin;
        vec3f interest = origin_state.interest;
        vec3f up = origin_state.up;

        linear3f newframe;
        //calc new frame
        newframe.vz = (interest == origin) ? vec3f(0, 0, 1) : -normalize(interest - origin);
        newframe.vx = cross(up, frame.vz);

        if (dot(newframe.vx, newframe.vx) < 1e-8f)
            newframe.vx = vec3f(0, 1, 0);
        else
            newframe.vx = normalize(newframe.vx);

        newframe.vy = normalize(cross(newframe.vz, newframe.vx));

        //Quaternion3f oldq(frame.vx, frame.vy, frame.vz);
        //Quaternion3f newq(newframe.vx, newframe.vy, newframe.vz);
        
        position = origin;
        upVector = up;
        frame = newframe;

        poiDistance = length(interest - origin);
        forceUpFrame();

    }

      
    /*! tilt the frame around the z axis such that the y axis is "facing upwards" */
    void forceUpFrame()
    {
      // frame.vz remains unchanged
      if (fabsf(dot(frame.vz,upVector)) < 1e-6f)
        // looking along upvector; not much we can do here ...
        return;
      frame.vx = normalize(cross(upVector,frame.vz));
      frame.vy = normalize(cross(frame.vz,frame.vx));
      modified = true;
    }

    void setUpVector(const vec3f &up)
    { 
        upVector = up; 
        forceUpFrame(); 
    }

    inline float computeStableEpsilon(float f) const
    {
      return abs(f) * float(1./(1<<21));
    }
                               
    inline float computeStableEpsilon(const vec3f v) const
    {
      return max(max(computeStableEpsilon(v.x),
                     computeStableEpsilon(v.y)),
                 computeStableEpsilon(v.z));
    }

    inline vec3f get_from() const { return position; }
    inline vec3f get_at() const { return getPOI(); }
    inline vec3f get_up() const { return upVector; }
      
    linear3f      frame         { one };
    vec3f         position      { 0,-1,0 };
    /*! distance to the 'point of interst' (poi); e.g., the point we
      will rotate around */
    float         poiDistance   { 1.f };
    vec3f         upVector      { 0,1,0 };
    /* if set to true, any change to the frame will always use to
       upVector to 'force' the frame back upwards; if set to false,
       the upVector will be ignored */
    bool          forceUp       { true };

    /*multiplier how fast the camera should move in world space*/
    float         motionSpeed   { 1.f };
    
    /*! gets set to true every time a manipulator changes the camera
      values */
    bool          modified      { true };

    struct { vec3f origin, interest, up; } origin_state;
  };



  // ------------------------------------------------------------------
  /*! abstract base class that allows to manipulate a renderable
    camera */
  struct CameraFrameManip {
    CameraFrameManip(CameraFrame *cameraFrame) : cameraFrame(cameraFrame) {}

    virtual void strafe(const vec3f &howMuch)
    {
      cameraFrame->position += howMuch;
      cameraFrame->modified =  true;
    }
    /*! strafe, in screen space */
    virtual void strafe(const vec2f &howMuch)
    {
      strafe(+howMuch.x*cameraFrame->frame.vx
             -howMuch.y*cameraFrame->frame.vy);
    }

    virtual void move(const float step) = 0;
    virtual void move_h(const float step) = 0;
    virtual void rotate(const float dx, const float dy) = 0;
    
    /*! mouse got dragged with left button pressed, by 'delta'
      pixels, at last position where */
    virtual void mouseDragLeft  (const vec2f &delta)
    {
      rotate(delta.x * degrees_per_drag_fraction,
             delta.y * degrees_per_drag_fraction);
    }
    
    /*! mouse got dragged with left button pressedn, by 'delta'
      pixels, at last position where */
    virtual void mousescroll (float delta)
    {
      move(delta*cameraFrame->motionSpeed*0.02f);
    }


    void reset() {
        cameraFrame->resetOrientation();
    }
      
  protected:
    CameraFrame *cameraFrame;
    const float kbd_rotate_degrees        {  10.f };
    const float degrees_per_drag_fraction { 150.f };
    const float pixels_per_move           {  10.f };
  };


  // ------------------------------------------------------------------
  /*! camera manipulator with the following traits
      
    - there is a "point of interest" (POI) that the camera rotates
    around.  (we track this via poiDistance, the point is then
    thta distance along the fame's z axis)
      
    - we can restrict the minimum and maximum distance camera can be
    away from this point
      
    - we can specify a max bounding box that this poi can never
    exceed (validPoiBounds).
      
    - we can restrict whether that point can be moved (by using a
    single-point valid poi bounds box
      
    - left drag rotates around the object

    - right drag moved forward, backward (within min/max distance
    bounds)

    - middle drag strafes left/right/up/down (within valid poi
    bounds)
      
  */
  struct InspectModeManip : public CameraFrameManip {

    InspectModeManip(CameraFrame *cameraFrame)
      : CameraFrameManip(cameraFrame)
    {}
      
  private:
    /*! helper function: rotate camera frame by given degrees, then
      make sure the frame, poidistance etc are all properly set,
      the widget gets notified, etc */
    virtual void rotate(const float deg_u, const float deg_v) override
    {
      float rad_u = -M_PI/180.f*deg_u;
      float rad_v = -M_PI/180.f*deg_v;

      CameraFrame &fc = *cameraFrame;
      
      const vec3f poi  = fc.getPOI();
      linear3f newframe
        = linear3f::rotate(fc.frame.vy,rad_u)
        * linear3f::rotate(fc.frame.vx,rad_v)
        * fc.frame;
      
      vec3f newposition = poi + fc.poiDistance * newframe.vz;

      if (newposition.y < 0.01f)return; //拒绝相机位移到水平面之下

      if (fc.forceUp) fc.forceUpFrame();

      fc.frame = newframe;
      if (fc.forceUp) fc.forceUpFrame();
      fc.position = newposition;
      fc.modified = true;

    }
      
    /*! helper function: move forward/backwards by given multiple of
      motion speed, then make sure the frame, poidistance etc are
      all properly set, the widget gets notified, etc */
    virtual void move(const float step) override
    {
      const vec3f poi = cameraFrame->getPOI();
      // inspectmode can't get 'beyond' the look-at point:
      const float minReqDistance = 0.1f * cameraFrame->motionSpeed;
      cameraFrame->poiDistance   = max(minReqDistance,cameraFrame->poiDistance-step);
      cameraFrame->position      = poi + cameraFrame->poiDistance * cameraFrame->frame.vz;
      cameraFrame->modified      = true;
    }

    virtual void move_h(const float step) override {}
  };

  // ------------------------------------------------------------------
  /*! camera manipulator with the following traits

    - left button rotates the camera around the viewer position

    - middle button strafes in camera plane
      
    - right buttton moves forward/backwards
      
  */
  struct FlyModeManip : public CameraFrameManip {

    FlyModeManip(CameraFrame *cameraFrame)
      : CameraFrameManip(cameraFrame)
    {}
      
  private:
    /*! helper function: rotate camera frame by given degrees, then
      make sure the frame, poidistance etc are all properly set,
      the widget gets notified, etc */
    virtual void rotate(const float deg_u, const float deg_v) override
    {
      float rad_u = -M_PI/180.f*deg_u;
      float rad_v = -M_PI/180.f*deg_v;

      CameraFrame &fc = *cameraFrame;
      
      gdt::Quaternion3d();
      fc.frame
        = linear3f::rotate(fc.frame.vy,rad_u)
        * linear3f::rotate(fc.frame.vx,rad_v)
        * fc.frame;

      if (fc.forceUp) fc.forceUpFrame();

      fc.modified = true;
    }
      
    /*! helper function: move forward/backwards by given multiple of
      motion speed, then make sure the frame, poidistance etc are
      all properly set, the widget gets notified, etc */
    virtual void move(const float step) override
    {
      cameraFrame->position    += step * cameraFrame->frame.vz;
      cameraFrame->modified     = true;
    }

    virtual void move_h(const float step) override
    {
        cameraFrame->position += step * cameraFrame->frame.vx;
        cameraFrame->modified = true;
    }
  };


  struct GLFWindow {
      GLFWindow(const std::string& title);
      ~GLFWindow();

      /*resize callback*/
      virtual void resize(const vec2i& newSize) {}

      /*key action callback*/
      virtual void keyAction(int key, int action, int mods) {}

      /*mouse scroll callback*/
      virtual void scrollAction(double offset) {}

      /*mouse move detect*/
      virtual void CameraMove() {}

      inline vec2i getMousePos() const
      {
          double x, y;
          glfwGetCursorPos(handle, &x, &y);
          return vec2i((int)x, (int)y);
      }

      /*render pixels with Optix SDK*/
      virtual void render() {}
      /*draw pixels on the screen ... */
      virtual void draw() {}

      void run();

      /*! the glfw window handle */
      GLFWwindow* handle{ nullptr };

  };


  struct GLFCameraWindow : public GLFWindow {
      GLFCameraWindow(const std::string& title,
          const vec3f& camera_from,
          const vec3f& camera_at,
          const vec3f& camera_up,
          const float worldScale)
          : GLFWindow(title),
          cameraFrame(worldScale)
      {
          cameraFrame.setOrientation(camera_from, camera_at, camera_up);
          enableFlyMode();
          enableInspectMode();
      }

      void enableFlyMode();
      void enableInspectMode();

      virtual void scrollAction(double offset) {
          if(cameraFrameManip == inspectModeManip)
              cameraFrameManip->mousescroll(static_cast<float>(offset));
      }

      virtual void CameraMove() {

          if (cameraFrameManip == flyModeManip) {
              if (isPressed.Wpressed) {
                  cameraFrameManip->move(-cameraFrame.motionSpeed * 0.02f);
              }
              else if (isPressed.Spressed) {
                  cameraFrameManip->move(cameraFrame.motionSpeed * 0.02f);
              }
              if (isPressed.Apressed) {
                  cameraFrameManip->move_h(-cameraFrame.motionSpeed * 0.02f);
              }
              else if (isPressed.Dpressed) {
                  cameraFrameManip->move_h(cameraFrame.motionSpeed * 0.02f);
              }
          }

          vec2i newPos = getMousePos();
          
          vec2i windowSize;
          glfwGetWindowSize(handle, &windowSize.x, &windowSize.y);
          if(isPressed.Altpressed)
            cameraFrameManip->mouseDragLeft(vec2f(newPos - lastMousePos) / vec2f(windowSize));
          
          lastMousePos = newPos;
      }

      struct {
          bool  Wpressed{ false }, Spressed{ false }, Apressed{ false }, Dpressed{ false }, 
              Altpressed{false};
      } isPressed;

      vec2i lastMousePos = { -1,-1 };

      friend struct CameraFrameManip;

      CameraFrame cameraFrame;
      std::shared_ptr<CameraFrameManip> cameraFrameManip;
      std::shared_ptr<CameraFrameManip> inspectModeManip;
      std::shared_ptr<CameraFrameManip> flyModeManip;

  };

  inline void GLFCameraWindow::enableFlyMode()
  {
    flyModeManip     = std::make_shared<FlyModeManip>(&cameraFrame);
    cameraFrameManip = flyModeManip;
  }
  
  inline void GLFCameraWindow::enableInspectMode()
  {
    inspectModeManip = std::make_shared<InspectModeManip>(&cameraFrame);
    cameraFrameManip = inspectModeManip;
  }
    
} // ::opz
