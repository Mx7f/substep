/**
  \file App.h

  The G3D 10.00 default starter app is configured for OpenGL 3.3 and
  relatively recent GPUs.
 */
#ifndef App_h
#define App_h
#include <G3D/G3DAll.h>

#ifdef G3D_WINDOWS
 // Compile RtAudio with windows direct sound support
#   define __WINDOWS_DS__
#endif
#include "RtAudio.h"

#include "CellularAutomata.h"

Array<float> g_currentAudioBuffer;
/** Application framework. */
class App : public GApp {
protected:
    RtAudio m_rtAudio;
    /** Settings for RtAudio. We never need to change the defaults */
    struct AudioSettings {
      int numChannels;
      int sampleRate;
      RtAudioFormat rtAudioFormat;
      
      AudioSettings() :
          numChannels(1),
          sampleRate(48000),
          rtAudioFormat(RTAUDIO_FLOAT32) {}
    } m_audioSettings;

    shared_ptr<GFont> m_guiFont;
    CellularAutomata m_automata;
    Color3 m_gridColor;

    bool m_showHelp;
    bool m_rainbowMode;

    /** Called from onInit */
    void makeGUI();

    void initializeAudio();
    
    void loadGrid();

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;
    virtual void onAI() override;
    virtual void onNetwork() override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;

    // You can override onGraphics if you want more control over the rendering loop.
    // virtual void onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) override;

    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;
    virtual void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D) override;

    void renderGUI(RenderDevice* rd);

    virtual bool onEvent(const GEvent& e) override;
    virtual void onUserInput(UserInput* ui) override;
    virtual void onCleanup() override;
    
    /** Sets m_endProgram to true. */
    virtual void endProgram();
};

#endif
