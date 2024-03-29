#include "Synthesizer.h"

shared_ptr<Synthesizer> Synthesizer::global = shared_ptr<Synthesizer>(new Synthesizer());

void Synthesizer::queueSound(shared_ptr<AudioSample> audioSample, int delay) {
    mutex.lock(); {
        m_sounds.append(SoundInstance(audioSample, -delay));
    } mutex.unlock();
}


void Synthesizer::synthesize(Array<Sample>& samples) {
    mutex.lock(); {
        int maxIndex = m_sounds.size() - 1;
        for (int i = maxIndex; i >= 0; --i) {
            if (m_sounds[i].play(samples)) {
                m_sounds.remove(i);
            }
        }
        sampleCount += double(samples.size());
    } mutex.unlock();
}