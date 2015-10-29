#include <G3D/G3DAll.h>
struct AudioSample {
  Array<float> buffer;
  /** In Hz */
  int sampleRate;
  static shared_ptr<AudioSample> createSine(int sampleRate, double frequency, int sampleCountDuration) {
    shared_ptr<AudioSample> s(new AudioSample());
    s->sampleRate = sampleRate;
    s->buffer.resize(sampleCountDuration);
    for (int i = 0; i < sampleCountDuration; ++i) {
      s->buffer[i] = sin( 2.0f * pif() * double(i) * frequency / sampleRate );
    }
  }
};

struct Sound {
  shared_ptr<AudioSample> audioSample;
  int currentPosition;
  /** Returns true if finished */ 
  bool play(Array<float>& buffer, int offset) {
    // TODO: double check this
    int maxSample = min(buffer.size(), audioSample->sampleCount() - currentPosition + offset);
    for(int i = offset; i < maxSample; ++i) {
      buffer[i] = audioSample->buffer[currentPosition];
      ++currentPosition;
    }
    return (currentPosition == audioSample->sampleCount());
  }
};

class Synthesizer {
 public:
  static unique_ptr<Synthesizer> global;
 private:
  Array<Sound> m_sounds;
  
};
