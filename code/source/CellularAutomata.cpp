#include "CellularAutomata.h"
#include "util.h"


static Vector2int16 vecFromDir(Direction d) {
    switch(d) {
    case Direction::UP:
        return Vector2int16(0,1);
    case Direction::DOWN:
        return Vector2int16(0,-1);
    case Direction::LEFT:
        return Vector2int16(-1,0);
    case Direction::RIGHT:
        return Vector2int16(1,0);
    default:
        alwaysAssertM(false, "Invalid direction");
        return Vector2int16(0, 0);
    }
}

static Direction rightOf(Direction d) {
    switch(d) {
    case Direction::UP:
        return Direction::RIGHT;
    case Direction::DOWN:
        return Direction::LEFT;
    case Direction::LEFT:
        return Direction::UP;
    case Direction::RIGHT:
        return Direction::DOWN;
    default:
        alwaysAssertM(false, "Invalid direction");
        return Direction::DOWN;
    }
}

static bool isVert(Direction d) {
    return d == Direction::UP || d == Direction::DOWN;
}


void CellularAutomata::onSimulation(double currentSampleCount, double sampleDelta) {
    if (m_paused) {
        return;
    }
    int oldBeatNum = beatNum();
    m_currentTime += float(sampleDelta) / m_sampleRate;
    int newBeatNum = beatNum();
    if (newBeatNum != oldBeatNum) {
        step();
    }
    for (const HeadWallCollision& c : m_wallCollisions) {
        int index = isVert(c.d) ? c.pos.x : c.pos.y;
        Synthesizer::global->queueSound(m_soundBank[index]);
    }
}

void CellularAutomata::step() {
    for (auto& head : m_playhead) {
        head.position += vecFromDir(head.direction);
    }
    for (int i = 0; i < m_playhead.size() - 1; ++i) {
        for (int j = i+1; j < m_playhead.size(); ++j) {
            if (m_playhead[i].position == m_playhead[j].position) {
                Direction& d0 = m_playhead[i].direction;
                Direction& d1 = m_playhead[j].direction;
                d0 = rightOf(d0);
                d1 = rightOf(d1);
                m_headCollisions.append(HeadHeadCollision(i,j,m_playhead[i].position));
            }
        }
    }


    for (auto& head : m_playhead) {
        Direction d = head.direction;
        if ( (Direction::LEFT == d) && (head.position.x == 0) ) {
            head.direction = Direction::RIGHT;
            m_wallCollisions.append(HeadWallCollision(d, head.position));
        } else if ( (Direction::RIGHT == d) && (head.position.x == m_width - 1) ) {
            head.direction = Direction::LEFT;
            m_wallCollisions.append(HeadWallCollision(d, head.position));
        } else if ( (Direction::UP == d) && (head.position.y == m_height - 1) ) {
            head.direction = Direction::DOWN;
            m_wallCollisions.append(HeadWallCollision(d, head.position));
        } else if ( (Direction::DOWN == d) && (head.position.y == 0) ) {
            head.direction = Direction::UP;
            m_wallCollisions.append(HeadWallCollision(d, head.position));
        }
    }
  
}

Vector2 CellularAutomata::normalizedCoord(const Vector2& position) {
    return position * Vector2(1.0f/m_width, 1.0f/m_height);
}

struct PlaneProperties {
    Vector2 dim;
    float z;
    PlaneProperties(float w, float h, float planeZ) : dim(w,h), z(planeZ) {}
};



Vector3 CellularAutomata::normCoordToPlane(const Vector2& norm) {
    PlaneProperties plane(float(4), float(4), 0.0f);
    return Vector3((norm - Vector2(0.5f, 0.5f))*plane.dim, plane.z);
}

// Borrowed from http://community.wolfram.com/groups/-/m/t/176327
Vector3 CellularAutomata::normCoordToTorus(const Vector2& norm) {
    float r1 = 2.0f; 
    float r2 = 1.0f;
    float theta = norm.x * 2.0f * pif();
    float phi   = norm.y * 2.0f * pif();

    Point3 p( (r1 + r2*cos(phi))*cos(theta),
                -r2*sin(phi),
                (r1 + r2*cos(phi))*sin(theta));

    return p;
}



Vector3 CellularAutomata::normCoordTo3DPoint(const Vector2& norm) {
    return  lerp(normCoordToPlane(norm), normCoordToTorus(norm), sin(m_currentTime*0.2)*0.5f+0.5f);
}

Vector3 CellularAutomata::normCoordTo3DPoint(float x, float y) {
    return normCoordTo3DPoint(Vector2(x, y));
}

void CellularAutomata::init(int width, int height, int numPlayHeads, int bpm, int sampleRate) {
    m_sampleRate    = sampleRate;
    m_bpm           = bpm;
    m_currentTime   = 0.0f;
    m_width         = width;
    m_height        = height;
    m_paused        = true;

    m_playhead.fastClear();
    Random& rnd = Random::common();
    for (int i = 0; i < numPlayHeads; ++i) {
    int x = rnd.integer(1, width-2);
    int y = rnd.integer(1, height-2);
    Direction d = Direction(rnd.integer(0,3));
        m_playhead.append(PlayHead(x,y, d));
    }

    m_soundBank.fastClear();
    Array<double> frequencies;
    Array<PianoKey> keys;
    keys.append(PianoKey::C, PianoKey::D, PianoKey::E, PianoKey::G, PianoKey::A);
    for (int i = 1; i <keys.size(); ++i) {
        frequencies.append(getFrequencyFromKey(keys[i], 3));
    }
    for (int i = 0; i <keys.size(); ++i) {
        frequencies.append(getFrequencyFromKey(keys[i], 4));
    }
    for (double frequency : frequencies) {
        float duration = 0.3f;
        float fadeOutProportion = 0.2f;
        m_soundBank.append(AudioSample::createSine(m_sampleRate, frequency, int(duration * m_sampleRate), fadeOutProportion));
    }
  
}

void CellularAutomata::drawSegmentedLine(RenderDevice* rd, const Color3& color, 
        const Vector2& start, const Vector2& end, int numSegments) {

    for (int i = 0; i < numSegments; ++i) {
        Vector2 p0 = lerp(start, end, float(i) / numSegments);
        Vector2 p1 = lerp(start, end, float(i+1) / numSegments);
        LineSegment line = LineSegment::fromTwoPoints(normCoordTo3DPoint(p0), normCoordTo3DPoint(p1));
        Draw::lineSegment(line, rd, color);
    }

    

}

void CellularAutomata::draw(RenderDevice* rd, const Rect2D& rect, const Color3& color) {
    float x0 = rect.x0();
    float x1 = rect.x1();
    float y0 = rect.y0();
    float y1 = rect.y1();

    int numSegments = 100;

    for (int i = 0; i < m_width; ++i) {
        float alpha = float(i)/(m_width - 1.0f);
        drawSegmentedLine(rd, color, Vector2(alpha, 0), Vector2(alpha, 1), 81);
    }

    for (int i = 0; i < m_height; ++i) {
        float alpha = float(i)/(m_height - 1.0f);
        drawSegmentedLine(rd, color, Vector2(0, alpha), Vector2(1, alpha), 81);

    }
    for (auto collision : m_wallCollisions) {
        if (isVert(collision.d)) {
            int i = collision.pos.x;
            float alpha = float(i) / (m_width - 1.0f);
            drawSegmentedLine(rd, color * 1.5f, Vector2(alpha, 0), Vector2(alpha, 1), 81);
        } else {
            int i = collision.pos.y;
            float alpha = float(i) / (m_height - 1.0f);
            drawSegmentedLine(rd, color * 1.5f, Vector2(0, alpha), Vector2(1, alpha), 81);
        }
    } 
  
    for (auto collision : m_headCollisions) {
        const Vector2int16 pos = collision.pos;
  

        Vector2 normalizedCoord = Vector2(pos) * Vector2(1.0f/(m_width - 1.0f), 1.0f/(m_height - 1.0f));



        const float x = lerp(x0, x1, normalizedCoord.x);
        const float y = lerp(y0, y1, normalizedCoord.y);

        const Vector2 c = Vector2(x,y);

        Draw::rect2D(Rect2D::xywh(Vector2(c)-Vector2(5,5), Vector2(10,10)), rd, color * 1.5);
    }
  
  
    Color4 clear = Color4::clear();
    const float sAlpha = stepAlpha();
    /*  debugPrintf("Step Alpha %f\n", sAlpha);
    debugPrintf("Time %f\n", m_currentTime);
    debugPrintf("Beat %d : %f\n", beatNum());*/
    for (auto head : m_playhead) {

        const Vector2int16 pos = head.position;
  
        Vector2 unnormalizedCoord = (Vector2(pos.x, pos.y) + Vector2(vecFromDir(head.direction)) * sAlpha);

        Vector2 normalizedCoord = unnormalizedCoord * Vector2(1.0f/(m_width - 1.0f), 1.0f/(m_height - 1.0f));


        Vector3 center = normCoordTo3DPoint(normalizedCoord);
        Vector3 radius(0.01f, 0.01f, 0.01f);
        Draw::box(AABox(center - radius, center + radius), rd, color*3.5f, Color4::clear());
    }

    m_wallCollisions.fastClear();
    m_headCollisions.fastClear();

}

