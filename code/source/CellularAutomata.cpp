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
  }
}

static bool isVert(Direction d) {
  return d == Direction::UP || d == Direction::DOWN;
}


void CellularAutomata::onSimulation(double currentSampleCount, double sampleDelta) {
  int oldBeatNum = beatNum();
  m_currentTime += sdt;
  int newBeatNum = beatNum();
  if (newBeatNum != oldBeatNum) {
    step();
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
  PlaneProperties plane(m_width, m_height, -1.0f);
  return Vector3((norm - Vector2(0.5f, 0.5f))*plane.dim, plane.z);
}

Vector3 CellularAutomata::normCoordToTorus(const Vector2& norm) {
  PlaneProperties plane(m_width, m_height, -1.0f);
  return Vector3((norm - Vector2(0.5f, 0.5f))*plane.dim, plane.z);
}


void CellularAutomata::init(int width, int height, int numPlayHeads, int bpm, int sampleRate) {
  m_bpm = bpm;
  m_currentTime = 0.0f;
  m_width = width;
  m_height = height;
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
    float duration = 0.01;
    m_soundBank.append(createSine(sampleRate, frequency, int(duration * sampleRate)));
  }
  
}

void CellularAutomata::draw(RenderDevice* rd, const Rect2D& rect, const Color3& color) {
  float x0 = rect.x0();
  float x1 = rect.x1();
  float y0 = rect.y0();
  float y1 = rect.y1();


  for (int i = 0; i < m_width; ++i) {
    float alpha = float(i)/(m_width - 1.0f);
    float x = lerp(x0, x1, alpha);
    LineSegment line = LineSegment::fromTwoPoints(Vector3(x,y0,0.0f), Vector3(x,y1,0.0f));    
    Draw::lineSegment(line, rd, color);

  }

  for (int i = 0; i < m_height; ++i) {
    float alpha = float(i)/(m_height - 1.0f);
    float y = lerp(y0, y1, alpha);
    LineSegment line = LineSegment::fromTwoPoints(Vector3(x0,y,0.0f), Vector3(x1,y,0.0f));    
    Draw::lineSegment(line, rd, color);

  }
  for (auto collision : m_wallCollisions) {
    if (isVert(collision.d)) {
      int i = collision.pos.x;
      float alpha = float(i)/(m_width - 1.0f);
      float x = lerp(x0, x1, alpha);
      LineSegment line = LineSegment::fromTwoPoints(Vector3(x,y0,0.0f), Vector3(x,y1,0.0f));    
      Draw::lineSegment(line, rd, color * 1.5f);
    } else {
      int i = collision.pos.y;
      float alpha = float(i)/(m_height - 1.0f);
      float y = lerp(y0, y1, alpha);
      LineSegment line = LineSegment::fromTwoPoints(Vector3(x0,y,0.0f), Vector3(x1,y,0.0f));    
      Draw::lineSegment(line, rd, color * 1.5f);
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



    const float x = lerp(x0, x1, normalizedCoord.x);
    const float y = lerp(y0, y1, normalizedCoord.y);

    Array<Vector2> points;
    const Vector2 c = Vector2(x,y);

    points.append(c + Vector2(1,1)*0.01f);
    points.append(c + Vector2(-1,1)*0.01f);
    points.append(c + Vector2(-1,-1)*0.01f);
    points.append(c + Vector2(1,-1)*0.01f);
    Draw::poly2D(points, rd, color*3.5f);
  }

  m_wallCollisions.fastClear();
  m_headCollisions.fastClear();

}

