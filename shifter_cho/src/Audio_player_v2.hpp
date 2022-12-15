#pragma once

#define    INITGUID //<== ��� ����� ��������� 3 ���� ���� ����� !!!!�� ������� ����� ��� �������!!!!
#define    DIRECTSOUND_VERSION        0x0800

#include <dsound.h>
#include <thread>
#include "AtomicQueue.hpp"

template <class T>
class Audio_player
{
public:
    Audio_player(AtomicQueue< T >* sndBuffer, const int bitsPerSample, const int sampleRate);
    ~Audio_player(); // �� ������������
    
    void start();
    void stop();
    
    int getSampleRate();    // �������� ������� �������������
    int getBitsPerSample(); // �������� ������� ��� ���������� � ����� ������

    //void close(); //TODO:void close() �� ��������, �� ������

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

//���������� �������
typedef unsigned char byte;
template class Audio_player<char>;  //���� ������� ��������� ������� ������
template class Audio_player<short>; //���� ������� ��������� ������� ������
template class Audio_player<byte>;  //���� ������� ��������� ������� ������