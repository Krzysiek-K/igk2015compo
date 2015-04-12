#include "FmodSound.hpp"
#include <fmod.hpp>
#include <cassert>

#pragma comment(lib, "fmodex_vc.lib")
#ifdef _DEBUG
    #define FMOD_ASSERT(expr)   do { FMOD_RESULT res__ = (expr); assert(res__ == FMOD_OK && #expr); } while(false)
#else
    #define FMOD_ASSERT(expr)   do { (expr); } while(false)
#endif

namespace Sound
{

System* g_System;

Sound::Sound(const char* filePath, float baseVolume, bool loop) :
    m_FilePath(filePath),
    m_BaseVolume(baseVolume),
    m_Loop(loop),
    m_Sound(NULL)
{
    if(g_System)
        Create();
}

Sound::~Sound()
{
    if(g_System)
        Destroy();
}

FMOD::Channel* Sound::Play(float volume, float pan)
{
    if(g_System == NULL || g_System->m_System == NULL || m_Sound == NULL)
        return NULL;

    FMOD::Channel *channel;
    FMOD_RESULT res = g_System->m_System->playSound(FMOD_CHANNEL_FREE, m_Sound, true, &channel);
    if (res == FMOD_OK)
    {
        if (volume != 1.f)
            FMOD_ASSERT( channel->setVolume(volume) );
        if (pan != 0.f)
            FMOD_ASSERT( channel->setPan(pan) );
        FMOD_ASSERT(channel->setPaused(false) );
        return channel;
    }
    else
    {
        assert(0);
        return NULL;
    }
}

void Sound::Stop(FMOD::Channel* channel)
{
    if(g_System == NULL || channel == NULL)
        return;

    bool isPlaying;
    FMOD_RESULT res = channel->isPlaying(&isPlaying);
    if (res == FMOD_OK)
    {
        if (isPlaying)
            FMOD_ASSERT( channel->stop() );
    }
    else
        assert(0);
}

void Sound::Create()
{
    if(g_System == NULL || g_System->m_System == NULL) return;

    FMOD_MODE mode = FMOD_2D | FMOD_SOFTWARE;
    mode |= m_Loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

    FMOD_RESULT res = g_System->m_System->createSound(m_FilePath.c_str(), mode, 0, &m_Sound);
    if (res == FMOD_OK)
    {
        if (m_BaseVolume != 1.f)
        {
            float frequency, volume, pan;
            int priority;
            res = m_Sound->getDefaults(&frequency, &volume, &pan, &priority);
            if (res == FMOD_OK)
            {
                FMOD_ASSERT( m_Sound->setDefaults(frequency, m_BaseVolume, pan, priority) );
            }
            else
                assert(0);
        }
    }
    else
        assert(0);
}

void Sound::Destroy()
{
    if(m_Sound)
    {
        m_Sound->release();
        m_Sound = NULL;
    }
}

System::System() :
    m_System(NULL),
    m_Music(NULL)
{
    assert(g_System == NULL);
    g_System = this;

    FMOD_RESULT res = FMOD::System_Create(&m_System);
    if (res == FMOD_OK)
    {
        res = m_System->init(100, FMOD_INIT_NORMAL, 0);
        if (res != FMOD_OK)
        {
            printf("FMOD initialization failed.\n");
            m_System->release();
            m_System = NULL;
        }
    }
    else
    {
        m_System = NULL;
        assert(false);
    }

    if(m_System)
    {
        for(Sound::iterator it = Sound::begin(); it != Sound::end(); ++it)
            it->Create();
    }
}

System::~System()
{
    for(Sound::iterator it = Sound::begin(); it != Sound::end(); ++it)
        it->Destroy();

    if (m_System) {
        m_System->release();
        m_System = 0;
    }

    g_System = NULL;
}

void System::Update()
{
    if(m_System)
        FMOD_ASSERT(m_System->update());
}

void System::StartMusic(const char* filePath, bool loop, float volume)
{
    if (m_System == 0) return;

    if (m_Music)
        m_Music->release();

    FMOD_MODE mode = FMOD_SOFTWARE;
    mode |= loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

    FMOD_RESULT res = m_System->createStream(filePath, mode, 0, &m_Music);
    if (res == FMOD_OK)
    {
        FMOD::Channel *channel;
        res = m_System->playSound(FMOD_CHANNEL_FREE, m_Music, true, &channel);
        if (res != FMOD_OK)
        {
            m_Music->release();
            m_Music = NULL;
        }
        else
        {
            if (volume != 1.f)
                FMOD_ASSERT( channel->setVolume(volume) );
            FMOD_ASSERT( channel->setPaused(false) );
        }
    }
    else
    {
        m_Music = 0;
        assert(0);
    }
}

void System::StopMusic()
{
    if(m_Music)
    {
        m_Music->release();
        m_Music = NULL;
    }
}

#if 0
void Initialize()
{
    if (g_FmodSystem) { assert(0); return; }

    FMOD_RESULT res = FMOD::System_Create(&g_FmodSystem);
    if (res != FMOD_OK) { assert(0); g_FmodSystem = 0; return; }
    
    res = g_FmodSystem->init(100, FMOD_INIT_NORMAL, 0);
    if (res != FMOD_OK)
    {
        printf("FMOD initialization failed.\n");
        g_FmodSystem->release();
        g_FmodSystem = 0;
    }
}

void Finalize()
{
    g_Music = 0;
    g_SoundMap.clear();

    if (g_FmodSystem) {
        g_FmodSystem->release();
        g_FmodSystem = 0;
    }
}

void Update()
{
    if (g_FmodSystem == 0) return;

    FMOD_ASSERT( g_FmodSystem->update() );
}

void LoadSound(const char *file_path, bool loop, float default_volume, float default_pan)
{
    if (g_FmodSystem == 0) return;

    std::string file_name = base::FilePathGetPart(file_path, false, true, false);
    if (file_name.empty()) { assert(0); return; }

    FMOD_MODE mode = FMOD_2D | FMOD_SOFTWARE;
    mode |= loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

    FMOD::Sound *sound;
    FMOD_RESULT res = g_FmodSystem->createSound(file_path, mode, 0, &sound);
    if (res == FMOD_OK) {
        if (default_volume != 1.f || default_pan != 0.f) {
            float frequency, volume, pan;
            int priority;
            res = sound->getDefaults(&frequency, &volume, &pan, &priority);
            if (res == FMOD_OK) {
                FMOD_ASSERT( sound->setDefaults(frequency, default_volume, default_pan, priority) );
            }
            else
                assert(0);
        }

        bool insert_succeeded = g_SoundMap.insert(SoundMap_t::value_type(file_name, sound)).second;
        if (!insert_succeeded) {
            sound->release();
            assert(0);
        }
    }
    else
        assert(0);
}

Channel_t StartSound(const char *file_name, float volume, float pan)
{
    if (g_FmodSystem == 0) return NullChannel;

    SoundMap_t::iterator it = g_SoundMap.find(std::string(file_name));
    if (it != g_SoundMap.end()) {
        FMOD::Channel *channel;
        FMOD_RESULT res = g_FmodSystem->playSound(FMOD_CHANNEL_FREE, it->second, false, &channel);
        if (res == FMOD_OK) {
            if (volume != 1.f)
                FMOD_ASSERT( channel->setVolume(volume) );
            if (pan != 0.f)
                FMOD_ASSERT( channel->setPan(pan) );
            FMOD_ASSERT(channel->setPaused(false) );
            return (Channel_t)channel;
        }
        else {
            assert(0);
            return NullChannel;
        }
    }
    else {
        assert(0);
        return NullChannel;
    }
}

void StopSound(Channel_t channel)
{
    if (channel == NullChannel) return;
    if (g_FmodSystem == 0) return;

    FMOD::Channel *fmod_channel = (FMOD::Channel*)channel;

    bool is_playing;
    FMOD_RESULT res = fmod_channel->isPlaying(&is_playing);
    if (res == FMOD_OK) {
        if (is_playing)
            FMOD_ASSERT( fmod_channel->stop() );
    }
    else
        assert(0);
}

void StartMusic(const char *file_path, bool loop, float volume)
{
    if (g_FmodSystem == 0) return;

    if (g_Music)
        g_Music->release();

    if (file_path && *file_path) {
        FMOD_MODE mode = FMOD_SOFTWARE;
        mode |= loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

        FMOD_RESULT res = g_FmodSystem->createStream(file_path, mode, 0, &g_Music);
        if (res == FMOD_OK) {
            FMOD::Channel *channel;
            res = g_FmodSystem->playSound(FMOD_CHANNEL_FREE, g_Music, false, &channel);
            if (res != FMOD_OK) {
                g_Music->release();
                g_Music = 0;
            }
            else
            {
                if (volume != 1.f)
                    FMOD_ASSERT( channel->setVolume(volume) );
            }
        }
        else {
            g_Music = 0;
            assert(0);
        }
    }
    else
        g_Music = 0;
}
#endif

} // namespace Sound
