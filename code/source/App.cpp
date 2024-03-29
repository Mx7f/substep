/** \file App.cpp */
#include "App.h"
#include "Synthesizer.h"
// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    {
        G3DSpecification g3dSpec;
        g3dSpec.audio = false;
        initGLG3D(g3dSpec);
    }

    GApp::Settings settings(argc, argv);

    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.caption             = argv[0];
    // settings.window.debugContext     = true;

    // settings.window.width              =  854; settings.window.height       = 480;
    // settings.window.width            = 1024; settings.window.height       = 768;
     settings.window.width            = 1280; settings.window.height       = 720;
//    settings.window.width               = 1920; settings.window.height       = 1080;
    // settings.window.width            = OSWindow::primaryDisplayWindowSize().x; settings.window.height = OSWindow::primaryDisplayWindowSize().y;
    settings.window.fullScreen          = false;
    settings.window.resizable           = ! settings.window.fullScreen;
    settings.window.framed              = ! settings.window.fullScreen;

    // Set to true for a significant performance boost if your app can't render at 60fps,
    // or if you *want* to render faster than the display.
    settings.window.asynchronous        = false;

    settings.depthGuardBandThickness    = Vector2int16(0, 0);
    settings.colorGuardBandThickness    = Vector2int16(0, 0);
    settings.dataDir                    = FileSystem::currentDirectory();
    settings.screenshotDirectory        = "../journal/";

    settings.renderer.deferredShading = false;
    settings.renderer.orderIndependentTransparency = false;


    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}


int audioCallback( void * outputBuffer, void * inputBuffer, unsigned int numFrames,
            double streamTime, RtAudioStreamStatus status, void * data ) {
    size_t numBytes = numFrames * sizeof(Sample);
    Array<Sample> output;
    output.resize(numFrames);
    for (Sample& s : output) {
        s = 0.0;
    }
    Synthesizer::global->synthesize(output);
    memcpy(outputBuffer, output.getCArray(), numBytes);
    return 0;
}



void App::initializeAudio() {

  unsigned int bufferByteCount = 0;
  unsigned int bufferFrameCount = 512;
    
  // Check for audio devices
  if( m_rtAudio.getDeviceCount() < 1 ) {
    // None :(
    debugPrintf("No audio devices found!\n");
    exit( 1 );
  }
  RtAudio::DeviceInfo info;

  unsigned int devices = m_rtAudio.getDeviceCount();


  // Let RtAudio print messages to stderr.
  m_rtAudio.showWarnings( true );

  // Set input and output parameters
  RtAudio::StreamParameters iParams, oParams;
  iParams.deviceId = m_rtAudio.getDefaultInputDevice();
  iParams.nChannels = m_audioSettings.numChannels;
  iParams.firstChannel = 0;
  oParams.deviceId = m_rtAudio.getDefaultOutputDevice();
  oParams.nChannels = m_audioSettings.numChannels;
  oParams.firstChannel = 0;
    
  // Create stream options
  RtAudio::StreamOptions options;

  g_currentAudioBuffer.resize(bufferFrameCount);
  try {
    // Open a stream
    m_rtAudio.openStream( &oParams, &iParams, m_audioSettings.rtAudioFormat, m_audioSettings.sampleRate, &bufferFrameCount, &audioCallback, (void *)&bufferByteCount, &options );
  } catch( RtAudioError& e ) {
    // Failed to open stream
    std::cout << e.getMessage() << std::endl;
    exit( 1 );
  }
  g_currentAudioBuffer.resize(bufferFrameCount);
  m_rtAudio.startStream();

}


void App::loadGrid() {
    Any grid;
    grid.load(System::findDataFile("grid.Grid.Any"));
    m_automata.init(grid["width"], grid["height"], grid["startHeads"], 150, m_audioSettings.sampleRate);
    m_gridColor = grid["color"];
}

// Called before the application loop begins.  Load data here and
// not in the constructor so that common exceptions will be
// automatically caught.
void App::onInit() {
    GApp::onInit();
    m_rainbowMode = true;
    setFrameDuration(1.0f / 60.0f);
    m_showHelp = true;
    m_guiFont = GFont::fromFile(System::findDataFile("console.fnt"));
    initializeAudio();
    
    loadGrid();
    // Call setScene(shared_ptr<Scene>()) or setScene(MyScene::create()) to replace
    // the default scene here.
    
    showRenderingStats      = true;

    makeGUI();
    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene("Test Scene");

}


void App::makeGUI() {
    // Initialize the developer HUD (using the existing scene)
    createDeveloperHUD();
    debugWindow->setVisible(false);
    developerWindow->videoRecordDialog->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    developerWindow->sceneEditorWindow->setVisible(false);
    developerWindow->setVisible(false);
    showRenderingStats = false;
    
    GuiPane* infoPane = debugPane->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
    infoPane->beginRow(); {
        infoPane->addButton("Pause", [this]() { m_automata.setPaused(!m_automata.paused()); });
        infoPane->addNumberBox("BPM", &m_automata.m_bpm, "", GuiTheme::LINEAR_SLIDER, 30, 300);
        infoPane->addEnumClassRadioButtons("Display Mode", &m_automata.m_displayMode);
    } infoPane->endRow();
    // Example of how to add debugging controls
    infoPane->pack();

    // More examples of debugging GUI controls:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugPane->addTextBox("Name", &myName);
    // debugPane->addNumberBox("height", &height, "m", GuiTheme::LINEAR_SLIDER, 1.0f, 2.5f);
    // button = debugPane->addButton("Run Simulator");

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}

void App::renderGUI(RenderDevice* rd) {

    m_guiFont->draw3DBillboard(rd, "Click to Add/Remove Playheads", Point3(0.0, 2.45, 0), 0.1f,
        m_gridColor, Color4::clear());
    m_guiFont->draw3DBillboard(rd, "Space to Play/Pause", Point3(0.0, 2.25, 0), 0.1f,
        m_gridColor, Color4::clear());
    m_guiFont->draw3DBillboard(rd, format("'H' to Toggle Help", m_automata.m_bpm), Point3(0.0, 2.125, 0), 0.1f,
        m_gridColor, Color4::clear());


    m_guiFont->draw3DBillboard(rd, format("%d BPM", m_automata.m_bpm), Point3(2.0, -2.125, 0), 0.1f, 
        m_gridColor, Color4::clear(), GFont::XALIGN_RIGHT);
    m_guiFont->draw3DBillboard(rd, format("<- -> ", m_automata.m_bpm), Point3(2.0, -2.25, 0), 0.1f,
        m_gridColor, Color4::clear(), GFont::XALIGN_RIGHT);
    
    
    m_guiFont->draw3DBillboard(rd, "'t' to Toggle 2D/3D Mode", Point3(-2.0, -2.125, 0), 0.1f,
        m_gridColor, Color4::clear(), GFont::XALIGN_LEFT);
    m_guiFont->draw3DBillboard(rd, "'r' to Toggle Rainbow Mode", Point3(-2.0, -2.25, 0), 0.1f,
        m_gridColor, Color4::clear(), GFont::XALIGN_LEFT);

}

void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    // This implementation is equivalent to the default GApp's. It is repeated here to make it
    // easy to modify rendering. If you don't require custom rendering, just delete this
    // method from your application and rely on the base class.

    if (! scene()) {
        return;
    }

    // Debug visualizations and post-process effects
    rd->pushState(m_framebuffer); {
        // Call to make the App show the output of debugDraw(...)
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
	
	    rd->setColorClearValue(Color3::black());
	    rd->clear();
        const Ray& mouseRay = scene()->eyeRay(activeCamera(), userInput->mouseXY() + Vector2(0.5f, 0.5f), rd->viewport(), Vector2int16(0, 0));
	    m_automata.draw(rd, mouseRay, m_gridColor);
        if (m_showHelp) {
            renderGUI(rd);
        }
        

    } rd->popState();

    if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!renderDevice->swapBuffersAutomatically())) {
        // We're about to render to the actual back buffer, so swap the buffers now.
        // This call also allows the screenshot and video recording to capture the
        // previous frame just before it is displayed.
        swapBuffers();
    }

	// Clear the entire screen (needed even though we'll render over it, since
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0));
}


void App::onAI() {
    GApp::onAI();
    // Add non-simulation game logic and AI code here
}


void App::onNetwork() {
    GApp::onNetwork();
    // Poll net messages here
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    float t = Synthesizer::global->currentSampleCount() * 0.0000001f;

    if (m_rainbowMode) {
        float newHue = Color3::toHSV(m_gridColor).x + sdt * 0.1f;
        m_gridColor = Color3::fromHSV(Vector3(newHue - floor(newHue), 1.0f, 1.0f));
    }

    m_automata.onSimulation(Synthesizer::global->currentSampleCount(), Synthesizer::global->tick());
    
    // Example GUI dynamic layout code.  Resize the debugWindow to fill
    // the screen horizontally.
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


bool App::onEvent(const GEvent& event) {
    // Handle super-class events
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F5)) { loadGrid(); }  

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::SPACE)) { 
        m_automata.setPaused(!m_automata.paused());
    }
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::LEFT)) {
        m_automata.m_bpm = max(m_automata.m_bpm-1, 1);
    }
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::RIGHT)) {
        m_automata.m_bpm = min(m_automata.m_bpm+1, 10000);
    }
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey('h'))) {
        m_showHelp = !m_showHelp;
    }
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey('t'))) {
        m_automata.m_displayMode = CellularAutomata::DisplayMode(1 - m_automata.m_displayMode);
    }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey('r'))) {
        m_rainbowMode = !m_rainbowMode;
    }
    if (GApp::onEvent(event)) { return true; }


    // If you need to track individual UI events, manage them here.
    // Return true if you want to prevent other parts of the system
    // from observing this specific event.
    //
    // For example,
    // if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_button)) { ... return true; }
    

    return false;
}


void App::onUserInput(UserInput* ui) {
    GApp::onUserInput(ui);
    bool pressed = ui->keyPressed(GKey::LEFT_MOUSE);
    bool held    = ui->keyDown(GKey::LEFT_MOUSE);
    const Ray& mouseRay = scene()->eyeRay(activeCamera(), userInput->mouseXY() + Vector2(0.5f, 0.5f), RenderDevice::current->viewport(), Vector2int16(0, 0));
    m_automata.handleMouse(pressed, held, mouseRay, userInput->mouseXY());
    
    (void)ui;
    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}


void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
    GApp::onPose(surface, surface2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
}


void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.
    Surface2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught.
    m_rtAudio.stopStream();
    if( m_rtAudio.isStreamOpen() )
        m_rtAudio.closeStream();

}


void App::endProgram() {
    m_endProgram = true;
}
