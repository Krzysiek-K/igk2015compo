#pragma once

#include <base.h>
#include <string>

namespace FMOD
{
    class System;
    class Sound;
    class Channel;
    typedef void* Channel_t;
}

namespace Sound
{

/*
Object of this class can be created before System is created, e.g. as global variable.
*/
class Sound : public base::Tracked<Sound>
{
public:
    Sound(const char* filePath, float baseVolume = 1.f, bool loop = false);
    ~Sound();

    // Pan: -1 (left) .. 0 (center) .. 1 (right)
    FMOD::Channel* Play(float volume = 1.f, float pan = 0.f);
    static void Stop(FMOD::Channel* channel);

private:
    void Create();
    void Destroy();

    std::string m_FilePath;
    float m_BaseVolume;
    bool m_Loop;

    FMOD::Sound* m_Sound;

    friend class System;
};

class System
{
public:
    // Initializes sound system.
    System();
    // Finalizes sound system.
    ~System();

    // Call every frame.
    void Update();

    // Starting new music stream replaces currently playing one.
    void StartMusic(const char* filePath, bool loop = true, float volume = 1.f);
    void StopMusic();

private:
    FMOD::System* m_System;
    // Not NULL means loaded and playing.
    FMOD::Sound* m_Music;

    friend class Sound;
};

extern System* g_System;

} // namespace Sound
