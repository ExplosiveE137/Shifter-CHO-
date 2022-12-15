#pragma once

#define	INITGUID //<== ��� ����� ��������� 3 ���� ���� ����� !!!!�� ������� ����� ��� �������!!!!
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
    Audio_recorder(AtomicQueue< T >* sndBuffer, const int bitsPerSample, const int sampleRate); //�� ������������
    ~Audio_recorder();  //�� ������������

    //void close(); //��������� ������ (���� ��� �������) + ����������� ������ � api ---- !�� ���������!

    void start();   //������ ������ ������� � ������ �������
    void stop();    //��������� ������ 

    int getSampleRate();    // �������� ������� �������������
    int getBitsPerSample(); // �������� ������� ��� ���������� � ����� ������
private:
    void run(); //����� ������� �������� � ��������� ������ � ���������� ������ � ������ ������� 
 
    LPDIRECTSOUNDCAPTURE8			g_pDSCapture; //��������� �� ��������� IDirectSoundCapture https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418154(v=vs.85)
    LPDIRECTSOUNDCAPTUREBUFFER8		g_pDSCaptureBuffer;
    WAVEFORMATEX	m_wfx; //��������� ���������� ������ �������� ������ https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex
    LPDIRECTSOUNDCAPTUREBUFFER	m_pDSCB; // ����� ������� 
    DSCBUFFERDESC	m_dsbd; //��������� DSCBUFFERDESC ��������� ����� �������.  https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee416823(v=vs.85)
                            //�� ������������ ������� IDirectSoundCapture8::CreateCaptureBuffer � �������� DirectSoundFullDuplexCreate8 .
    
    float m_bufferDurationInTime;
    AtomicQueue<T>* m_sndBuffer;  //��� �� �����, � ���������� �������� ��������� �� �����
    bool m_cmd_stop_thread = true;
    bool m_thread_stopped = true;
};

typedef unsigned char byte;
template class Audio_recorder <char>;   //���� ������� ��������� ������� ������
template class Audio_recorder <short>;  //���� ������� ��������� ������� ������
template class Audio_recorder <byte>;   //���� ������� ��������� ������� ������