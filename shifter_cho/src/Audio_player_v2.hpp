#pragma once

#define    INITGUID //<== эта др€нь потратила 3 часа моей жизни !!!!Ќ≈ “–ќ√ј“№ »Ќј„≈ ¬—≈ —ƒќ’Ќ≈“!!!!
#define    DIRECTSOUND_VERSION        0x0800

#include <dsound.h>
#include <thread>
#include "AtomicQueue.hpp"

template <class T>
class Audio_player
{
public:
    Audio_player(AtomicQueue< T >* sndBuffer, const int bitsPerSample, const int sampleRate);
    ~Audio_player(); // не используетс€
    
    void start();
    void stop();
    
    int getSampleRate();    // получить частоту дискретизации
    int getBitsPerSample(); // получить сколько бит содержитс€ в одном семпле

    //void close(); //TODO:void close() не работает, не готово

private:
    void run();
    
    DSBUFFERDESC m_dsbd;
    WAVEFORMATEX m_wfx;
    LPDIRECTSOUND m_lpDS;
    LPDIRECTSOUNDBUFFER m_lpDSB;

    float m_bufferDurationInTime;

    AtomicQueue< T > * m_sndBuffer;
    bool m_cmd_stop_thread = false;
    bool m_thread_stopped = false;
};

//заполнение шаблона
typedef unsigned char byte;
template class Audio_player<char>;  //€вно создаем экземпл€р шаблона класса
template class Audio_player<short>; //€вно создаем экземпл€р шаблона класса
template class Audio_player<byte>;  //€вно создаем экземпл€р шаблона класса