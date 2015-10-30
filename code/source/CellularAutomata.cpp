#include "CellularAutomata.h"
#include "util.h"

static bool intersects(const Ray& mouseRay, const Vector3& center, float radius) {
    return mouseRay.intersectionTime(Sphere(center, radius)) < 10000.0f;
}

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

static float slew(float value, float goal, float delta, float alpha) {
    return (goal - value)*alpha*delta + value;
}

void CellularAutomata::onSimulation(double currentSampleCount, double sampleDelta) {
    SimTime deltaTime = float(sampleDelta) / m_sampleRate;
    m_displayInterpolationFactor = slew(m_displayInterpolationFactor, m_displayMode, deltaTime, 0.8f);
    if (m_paused) {
        m_currentTime = floor(m_currentTime / stepTime())*stepTime();
        return;
    }
    int oldBeatNum = beatNum();
    m_currentTime += deltaTime;
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
    return lerp(normCoordToPlane(norm), normCoordToTorus(norm), m_displayInterpolationFactor);
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

void CellularAutomata::handleMouse(bool isPressed, bool isDown, const Ray & mouseRay, const Vector2 & mousePos) {
    m_transientPlayhead.position = Vector2int16(-5, -5);
    if (m_paused) {
        Point2int16 selectedPosition(-1,-1);
        for (int x = 0; x < m_width; ++x) {
            for (int y = 0; y < m_height; ++y) {
                Vector2 normalizedCoord = Vector2(x,y) * 
                    Vector2(1.0f / (m_width - 1.0f), 1.0f / (m_height - 1.0f));
                     
                const Point3& p = normCoordTo3DPoint(normalizedCoord);
                if (intersects(mouseRay, p, collisionRadius())) {
                    selectedPosition = Vector2int16(x, y);
                }
            }
        }
        
        if (selectedPosition.x >= 0) {
 
            m_transientPlayhead.position  = selectedPosition;
            m_transientPlayhead.direction = Direction::DOWN;

            // Super hackish way to select the direction to point in based on mouse position 
            // from the center (should work on in either mode, but only tested on 2D)
            // This is the first thing due for a cleanup
            float minDistance = 10.0f; // Basically infinity...
            Vector2 normalizedCoord = Vector2(selectedPosition) *
                Vector2(1.0f / (m_width - 1.0f), 1.0f / (m_height - 1.0f)); 
            const Point3& center = normCoordTo3DPoint(normalizedCoord);
            float t = mouseRay.intersectionTime(Sphere(center, collisionRadius()));
            Point3 intersectionPoint = mouseRay.origin() + mouseRay.direction() * t;
            for (int i = 0; i < 4; ++i) {
                Vector2 npos = normalizedCoord + Vector2(vecFromDir(Direction(i)))*0.001f;
                const Point3& p = normCoordTo3DPoint(npos);
                if ((intersectionPoint - p).length() < minDistance) {
                    minDistance = (intersectionPoint - p).length();
                    m_transientPlayhead.direction = Direction(i);
                }
            }

            if (isPressed) {
                bool removed = false;
                for (int i = m_playhead.size() - 1; i >= 0; --i) {
                    if (selectedPosition == m_playhead[i].position) {
                        m_playhead.remove(i);
                        removed = true;
                        break;
                    }
                }
                
                if (!removed) {
                    m_playhead.append(m_transientPlayhead);
                }
                
            }
            
        } 
        
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

void CellularAutomata::draw(RenderDevice* rd, const Ray& mouseRay, const Color3& color) {

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

        Vector3 center = normCoordTo3DPoint(normalizedCoord);

        Vector3 radius(0.01f, 0.01f, 0.01f);
        Draw::box(AABox(center - radius, center + radius), rd, color*3.5f, Color4::clear());
    }
  
  
    Color4 clear = Color4::clear();
    const float sAlpha = stepAlpha();
    /*  debugPrintf("Step Alpha %f\n", sAlpha);
    debugPrintf("Time %f\n", m_currentTime);
    debugPrintf("Beat %d : %f\n", beatNum());*/
    bool showTransientPlayhead = m_paused;
    for (auto head : m_playhead) {

        const Vector2int16 pos = head.position;
  
        Vector2 unnormalizedCoord = (Vector2(pos.x, pos.y) + Vector2(vecFromDir(head.direction)) * sAlpha);

        Vector2 normalizedCoord = unnormalizedCoord * Vector2(1.0f/(m_width - 1.0f), 1.0f/(m_height - 1.0f));

        Vector3 center = normCoordTo3DPoint(normalizedCoord);

        if (m_paused) { 
            float colorMultiplier = 1.0f;
            if (pos == m_transientPlayhead.position) {
                showTransientPlayhead = false;
                colorMultiplier = 1.5f;
            }
            Draw::arrow(center, (normCoordTo3DPoint(normalizedCoord + Vector2(vecFromDir(head.direction))) - center)*0.02f, rd, color*colorMultiplier, 0.5f);
        } else {
            float maxDimension = max(m_width - 1.0f, m_height - 1.0f);
            float r = 0.0007f*maxDimension;
            Vector3 radius(r, r, r);
            Draw::box(AABox(center - radius, center + radius), rd, color*3.5f, Color4::clear());
        }
    }
    if (showTransientPlayhead && m_transientPlayhead.position.x >= 0) {
        Point2 normalizedCoord = Point2(m_transientPlayhead.position) * Vector2(1.0f / (m_width - 1.0f), 1.0f / (m_height - 1.0f));
        const Point3& center = normCoordTo3DPoint(normalizedCoord);
        Draw::arrow(center, (normCoordTo3DPoint(normalizedCoord + Vector2(vecFromDir(m_transientPlayhead.direction))) - center)*0.02f, rd, Color4(color, 0.5f), 0.5f);
    }

    m_wallCollisions.fastClear();
    m_headCollisions.fastClear();

}

