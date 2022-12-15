#pragma once

#define	INITGUID //<== эта дрянь потратила 3 часа моей жизни !!!!НЕ ТРОГАТЬ ИНАЧЕ ВСЕ СДОХНЕТ!!!!
#define	DIRECTSOUND_VERSION		0x0800

#include <dsound.h>
#include <thread>
#include <chrono>
#include <iostream>
#include "AtomicQueue.hpp"

template <class T>
class Audio_recorder
{
public:
    Audio_recorder(AtomicQueue< T >* sndBuffer, const int bitsPerSample, const int sampleRate); //не используется
    ~Audio_recorder();  //не используется

    //void close(); //Остановка записи (если она ведется) + заверешение работы с api ---- !не требуется!

    void start();   //Запуск записи семплов в атомик очередь
    void stop();    //Остановка записи 

    int getSampleRate();    // получить частоту дискретизации
    int getBitsPerSample(); // получить сколько бит содержится в одном семпле
private:
    void run(); //Метод который работает в отдельном потоке и записывает семплы в атомик очередь 
 
    LPDIRECTSOUNDCAPTURE8			g_pDSCapture; //указатель на интерфейс IDirectSoundCapture https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418154(v=vs.85)
    LPDIRECTSOUNDCAPTUREBUFFER8		g_pDSCaptureBuffer;
    WAVEFORMATEX	m_wfx; //структура определяет формат звуковых данных https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex
    LPDIRECTSOUNDCAPTUREBUFFER	m_pDSCB; // буфер захвата 
    DSCBUFFERDESC	m_dsbd; //Структура DSCBUFFERDESC описывает буфер захвата.  https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee416823(v=vs.85)
                            //Он используется методом IDirectSoundCapture8::CreateCaptureBuffer и функцией DirectSoundFullDuplexCreate8 .
    
    float m_bufferDurationInTime;
    AtomicQueue<T>* m_sndBuffer;  //это не буфер, а переменная хранящая указатель на буфер
    bool m_cmd_stop_thread = true;
    bool m_thread_stopped = true;
};

typedef unsigned char byte;
template class Audio_recorder <char>;   //явно создаем экземпляр шаблона класса
template class Audio_recorder <short>;  //явно создаем экземпляр шаблона класса
template class Audio_recorder <byte>;   //явно создаем экземпляр шаблона класса