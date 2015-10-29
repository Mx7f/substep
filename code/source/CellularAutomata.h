#ifndef CellularAutomata_h
#define CellularAutomata_h
#include <G3D/G3DAll.h>
#include "Synthesizer.h"
G3D_DECLARE_ENUM_CLASS(Direction, UP, DOWN, LEFT, RIGHT);


struct PlayHead {
    Vector2int16 position;
    Direction direction;

    PlayHead() : position(Vector2int16(0,0)), direction(Direction::RIGHT) {}
    PlayHead(int x, int y, Direction d = Direction::RIGHT) : position(Vector2int16(x,y)), direction(d){}
};


class CellularAutomata {
    struct HeadHeadCollision {
        int i0;
        int i1;
        Vector2int16 pos;
        HeadHeadCollision() {}
        HeadHeadCollision(int ind0, int ind1, Vector2int16 p) : 
            i0(ind0), i1(ind1), pos(p) {}
    
    };
    struct HeadWallCollision {
        Direction d;
        Vector2int16 pos;
        HeadWallCollision() {}
        HeadWallCollision(Direction dir, Vector2int16 p) :
            d(dir), pos(p) {}
    };
    Array<HeadWallCollision> m_wallCollisions;
    Array<HeadHeadCollision> m_headCollisions;

    SimTime m_currentTime;
    
    int    m_sampleRate;
    int    m_width;
    int    m_height;

    bool m_paused;

    Array<PlayHead> m_playhead; 
    Array<shared_ptr<AudioSample>> m_soundBank;
  
    Vector2 normalizedCoord(const Vector2& position);

    Vector3 normCoordToPlane(const Vector2& norm);

    Vector3 normCoordTo3DPoint(float x, float y);
    Vector3 normCoordTo3DPoint(const Vector2& norm);

    void drawSegmentedLine(RenderDevice* rd, const Color3& color,
        const Vector2& start, const Vector2& end, int numSegments);

    Vector3 normCoordToTorus(const Vector2& norm);
  
    SimTime stepTime() const {
        return (60.0f/(m_bpm)) / 2.0f;
    }

    int beatNum() const {
        return (int)(floor(m_currentTime/stepTime()));
    }

    float stepAlpha() const {
        return (m_currentTime - (beatNum() * stepTime())) / stepTime();
    }

    void step();
public:
    int    m_bpm;
    void draw(RenderDevice* rd, const Rect2D& rect, const Color3& color);
    void onSimulation(double currentSampleCount, double sampleDelta);
    void init(int width, int height, int numPlayHeads = 0, int bpm = 150, int sampleRate = 48000);
    bool paused() const {
        return m_paused;
    }
    void setPaused(bool b) {
        m_paused = b;
    }
};
#endif
