#include "Audio_player_v2.hpp"

template <class T>
int Audio_player<T>::getSampleRate() {
	return m_wfx.nSamplesPerSec;
}

template <class T>
int Audio_player<T>::getBitsPerSample() {
	return m_wfx.wBitsPerSample;
}

template <class T>
Audio_player<T>::Audio_player(AtomicQueue< T >* b, const int bitsPerSample, const int sampleRate){
	m_sndBuffer = b;                                      //это не буфер, а переменная хранящая указатель на буфер

	m_cmd_stop_thread = false;  //переменная завершения цикла
	m_thread_stopped = false;   //подтверждение завершения цикла

	if (FAILED(DirectSoundCreate(NULL, &m_lpDS, NULL))) {
		printf("DirectSoundCreate ERROR!\n");
		exit(-99);
	}
	//Set Cooperative Level
	HWND hWnd = GetForegroundWindow();
	if (hWnd == NULL)
	{
		hWnd = GetDesktopWindow();
	}

	if (FAILED(m_lpDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY))) {
		printf("SetCooperativeLevel ERROR!\n");
		return;
	}

	//Create Primary Buffer 
	
	ZeroMemory(&m_dsbd, sizeof(m_dsbd));
	m_dsbd.dwSize = sizeof(DSBUFFERDESC);
	m_dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	m_dsbd.dwBufferBytes = 0;
	m_dsbd.lpwfxFormat = NULL;

	LPDIRECTSOUNDBUFFER lpDSB = NULL;
	if (FAILED(m_lpDS->CreateSoundBuffer(&m_dsbd, &lpDSB, NULL))) {
		printf("CreateSoundBuffer Primary Buffer ERROR!\n");
		return;
	}

	memset(&m_wfx, 0, sizeof(WAVEFORMATEX));
	m_wfx.wFormatTag = WAVE_FORMAT_PCM;
	m_wfx.nChannels = 1;
	m_wfx.nSamplesPerSec = sampleRate; //иногда стоить сделать чуть быстрее записи
	m_wfx.wBitsPerSample = bitsPerSample;
	m_wfx.nBlockAlign = m_wfx.nChannels * (m_wfx.wBitsPerSample/8);
	m_wfx.nAvgBytesPerSec = m_wfx.nSamplesPerSec * (m_wfx.wBitsPerSample / 8);
	m_wfx.cbSize = 0;

	//Set Primary Buffer Format
	HRESULT hr = 0;
	if (FAILED(lpDSB->SetFormat(&m_wfx))) {
		printf("Set Primary Buffer Format ERROR!\n");
		return;
	}

	//Create Second Sound Buffer
	m_bufferDurationInTime = 0.2; // Оптим 0.2 длительность буфера звуковоспроизведения в секундах (задержка звука = половине длит буф)
	m_dsbd.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS;
	m_dsbd.dwBufferBytes = m_bufferDurationInTime * m_wfx.nAvgBytesPerSec; //x Seconds Buffer
	m_dsbd.lpwfxFormat = &m_wfx;

	if (FAILED(m_lpDS->CreateSoundBuffer(&m_dsbd, &m_lpDSB, NULL))) {
		printf("Create Second Sound Buffer Failed!");
		return;
	}
	printf("Audio_player: init success!\n");
}

template <class T>
Audio_player<T>::~Audio_player(){}

template <class T>
void Audio_player<T>::start(){
	m_cmd_stop_thread = false;
	std::thread body_thread(&Audio_player::run, this);  //запускаем поток выполняющий функцию run по указателю, этого экземпляра класса this
    body_thread.detach();                               //отключились от потока
}

template <class T>
void Audio_player<T>::run()
{
	bool turn_on = false; //принудительно выключили плейер

	DWORD writePosition = 0; //Смещение с которого мы будем записывать байты (от начала буфера)
	//DWORD desiredWriteSamples = m_dsbd.dwBufferBytes / 4; //Половина буфера в семлах //В ЭТОМ И БЫЛ КОСЯК

	DWORD writeCursor = 0; //за ним можно безопасно писать данные (не используем)
	DWORD playCursor = 0; //Курсор на семлах которые сейчас воспроизводятся

	T* pAudio1 = NULL; //Адрес переменной, которая получает указатель на заблокированную часть буфера.
	DWORD dwBytesAudio1 = 0; //Адрес переменной, которая получает количество байтов в блоке в ppvAudioPtr1
	
	HRESULT hr = 0; //Хранит код ошибки (для дебага в отладчике)

	bool flag = 0; //КАКУЮ ПОЛОВИНУ БУФЕРА БУДЕМ ПИСАТЬ (0 - вторая, 1 - первая)

	T sample_byte;

	printf("Audio_player: is start!\n");
	while(!m_cmd_stop_thread){
		if (m_sndBuffer->Size() < m_dsbd.dwBufferBytes / 2) {//кол-во байт в очереди меньше половины буфера //И тут был косяк
			std::this_thread::sleep_for(std::chrono::milliseconds(50)); //тогда спим 50мс, ждем когда в очередь набьется достаточно семплов
			//printf("Audio_player: sndBuff is empty!\n");
			m_lpDSB->Stop();
		}
		else m_lpDSB->Play(0, 0, DSBPLAY_LOOPING); //начинает цикличное воспроизведение данных из буфера

		m_lpDSB->GetCurrentPosition(&playCursor, &writeCursor);
		if (flag == 0) {
			if (playCursor <= m_dsbd.dwBufferBytes / 2) {
				writePosition = m_dsbd.dwBufferBytes / 2;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds((long long)(m_bufferDurationInTime / 4 * 1000)));
				continue;
			}
		}
		else {
			if (playCursor > m_dsbd.dwBufferBytes / 2) {
				writePosition = 0;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds((long long)(m_bufferDurationInTime / 4 * 1000)));
				continue;
			}
		}

		hr = m_lpDSB->Lock(writePosition, m_dsbd.dwBufferBytes / 2, (LPVOID*)&pAudio1, &dwBytesAudio1, 0, 0, 0);

		for (int i = 0; i < dwBytesAudio1 / sizeof(T); i++) {
			m_sndBuffer->Pull(sample_byte);
			//fwrite(&sample_byte, 1, 1, recfile2);
			pAudio1[i] = sample_byte;
		}

		hr = m_lpDSB->Unlock(pAudio1, dwBytesAudio1, NULL, NULL);
		flag = !flag;
		std::this_thread::sleep_for(std::chrono::milliseconds((long long)(m_bufferDurationInTime / 4 * 1000)));
		//printf("debug stop audio player!\n");
		//break;
		}
    fprintf(stderr, "Audio_player: m_cmd_stop_thread: %d\n", m_cmd_stop_thread);  //пришла команда стоп, вышли из цикла
    m_thread_stopped = true;  //если цикл завершен, даем обратную связь
}

template <class T>
void Audio_player<T>::stop()
{
    m_cmd_stop_thread = true;           //даем команду остановить цикл

	m_lpDSB->Stop(); //останавливает воспроизведение

    size_t temp_count = 0;
    while (m_thread_stopped == false)   //контролируем останов
    {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        temp_count++;
        if (temp_count == 200)
        {
            fprintf(stderr, "player: ERROR: m_thread_stopped: %d\n", m_thread_stopped);
            exit(1);
        }
    }
}

//template <class T>
//void Audio_player<T>::close(){
//	stop();
//}