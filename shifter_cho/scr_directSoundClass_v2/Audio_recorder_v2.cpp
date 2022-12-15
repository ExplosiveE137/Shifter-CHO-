#include "Audio_recorder_v2.hpp"

using namespace std::chrono_literals;

template <class T>
Audio_recorder<T>::Audio_recorder(AtomicQueue< T >* sndBuffer, const int bitsPerSample, const int sampleRate){
	m_sndBuffer = sndBuffer;      //это не буфер, а переменная хранящая указатель на буфер
	if (FAILED(DirectSoundCaptureCreate8(NULL, &g_pDSCapture, NULL))) {  //создает и инициализирует объект, поддерживающий интерфейс IDirectSoundCapture8
		printf("DirectSoundCaptureCreate8 ERROR!\n");
		exit(-99);
	}
	//параметры
	//lpcGUID	//	Адрес GUID, который идентифицирует устройство звукозаписи.Значение этого параметра должно быть одним из идентификаторов GUID, возвращаемых DirectSoundCaptureEnumerate, или NULL для устройства по умолчанию, или одним из следующих значений.
	//	lplpDSC	//	Адрес переменной для получения указателя интерфейса IDirectSoundCapture8 .
	//	pUnkOuter	//	Адрес интерфейса IUnknown управляющего объекта для агрегации COM.Должно быть NULL, так как агрегация не поддерживается.

	//НАСТРОЕЧКИ ЗАХВАТА закладываемые на этапе компиляции//////////////////////////////////////////////
	m_wfx.wFormatTag = WAVE_FORMAT_PCM; //Данные PCM (импульсно-кодовая модуляция) в целочисленном формате со знаком, little endian
	m_wfx.nChannels = 1; //Кол-во каналов
	m_wfx.nSamplesPerSec = sampleRate; //Частота дискретизации
	m_wfx.wBitsPerSample = bitsPerSample; //Кол-во бит на семпл
	m_wfx.nBlockAlign = m_wfx.nChannels * (m_wfx.wBitsPerSample / 8); //Выравнивание, не трогать, расчитывается автоматически
	m_wfx.nAvgBytesPerSec = m_wfx.nSamplesPerSec * m_wfx.nBlockAlign; //не трогать
	m_wfx.cbSize = 0; //Размер в байтах дополнительной информации о формате, добавленной в конец структуры WAVEFORMATEX 
	m_bufferDurationInTime = 0.2; //Сколько секунд записи помещает буфер (задежка передачи в атомик  >= половине размера буфера во времени)
	////////////////////////////////////////////////////////////////////////////////////////////////////

	ZeroMemory(&m_dsbd, sizeof(DSCBUFFERDESC)); //memset макрос, "на всякий зануляем"
	m_dsbd.dwSize = sizeof(DSCBUFFERDESC); //Размер структуры в байтах. Этот член должен быть инициализирован перед использованием структуры.

	m_dsbd.dwBufferBytes = m_bufferDurationInTime * m_wfx.nAvgBytesPerSec; //Размер создаваемого буфера захвата в байтах. 
	m_dsbd.lpwfxFormat = &m_wfx; //Указатель на структуру WAVEFORMATEX , содержащую формат для захвата данных.

	g_pDSCapture->CreateCaptureBuffer(&m_dsbd, &m_pDSCB, NULL); //создает буфер захвата
	//Параметры
	//	lpDSCBufferDesc
	//	Указатель на структуру DSCBUFFERDESC, содержащую значения для создаваемого буфера захвата.
	//	lplpDirectSoundCaptureBuffer
	//	Адрес указателя интерфейса IDirectSoundCaptureBuffer в случае успеха.
	//	pUnkOuter
	//	Управление IUnknown агрегата.Его значение должно быть NULL.
	m_pDSCB->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&g_pDSCaptureBuffer); //https://docs.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-queryinterface(refiid_void)
	m_pDSCB->Release(); //https://docs.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-release
}
template <class T>
Audio_recorder<T>::~Audio_recorder(){}

template <class T>
int Audio_recorder<T>::getSampleRate() {
	return m_wfx.nSamplesPerSec;
}

template <class T>
int Audio_recorder<T>::getBitsPerSample() {
	return m_wfx.wBitsPerSample;
}

template <class T>
void Audio_recorder<T>::start(){
	if (m_thread_stopped == false) {
		fprintf(stderr, "recorder: ERROR! the stream is already running! Exit...\n");
		exit(1);
	}
	m_cmd_stop_thread = false;
	g_pDSCaptureBuffer->Start(DSCBSTART_LOOPING); //начинает захват данных в буфер циклично https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418181(v=vs.85)
	//запускаем поток выполняющий функцию run по указателю, этого экземпляра класса this
	std::thread body_thread(&Audio_recorder::run, this);
	body_thread.detach();//отключились от потока
	m_thread_stopped = false;
	std::cout << "recorder: start()\n";
}

template <class T>
void Audio_recorder<T>::run()
{
	T* pAudio1 = NULL; //Адрес переменной, которая получает указатель на первую заблокированную часть буфера.
	DWORD AudioBytes1 = NULL; //Адрес переменной, которая получает количество байтов в блоке в ppvAudioPtr1

	DWORD readPosition=0; //Смещение с которого мы будем считывать байты (от начала буфера)
	DWORD desiredReadBytes = m_dsbd.dwBufferBytes / 2; //Сколько байт хотим считать (половина буфера)

	DWORD readСursor=0; //Курсор, ПЕРЕД КОТОРЫМ МОЖНО БЕЗОПАСНО ЧИТАТЬ БАЙТЫ
	DWORD captureСursor=0; // курсор захвата (У НАС НЕ ИСПОЛЬЗУЕТСЯ ВООБЩЕ)
	
	bool flag = 0; //КАКУЮ ПОЛОВИНУ БУФЕРА БУДЕМ ЧИТАТЬ (0 - первая, 1 - вторая)

	//Сначала идет курсора захвата, потом курсор чтения, мы можем безопасно брать семплы до адреса курсора чтения
    while (!m_cmd_stop_thread) //пока нет команды стоп
    {
		g_pDSCaptureBuffer->GetCurrentPosition(&captureСursor, &readСursor);
		if (flag == 0) {
			if (readСursor >= m_dsbd.dwBufferBytes/2) {
				readPosition = 0;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds((long long)(m_bufferDurationInTime/4 * 1000)));
				continue;
			} 
		}
		else {
			if (readСursor < m_dsbd.dwBufferBytes / 2) {
				readPosition = m_dsbd.dwBufferBytes / 2;

			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds((long long)(m_bufferDurationInTime/4 * 1000)));
				continue;
			}
		}

		g_pDSCaptureBuffer->Lock(readPosition, desiredReadBytes,
			(LPVOID*)&pAudio1, &AudioBytes1,
			NULL, NULL,
			0); https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418179(v=vs.85)
				//блокирует часть буфера.Блокировка буфера возвращает указатели на буфер, 
				//позволяя приложению считывать или записывать аудиоданные в память.
				//также считывает кусок буффера с p, размером s
	
		for (unsigned int i = 0; i < AudioBytes1/sizeof(T); i++)
		{
			m_sndBuffer->Push(pAudio1[i]); //пишем байты из внутреннего буфера в конец очереди по одному
		}

		g_pDSCaptureBuffer->Unlock(pAudio1, AudioBytes1, NULL, NULL); //https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418185(v=vs.85)
		std::this_thread::sleep_for(std::chrono::milliseconds((long long)(m_bufferDurationInTime / 4 * 1000)));
		flag = !flag;
    }
    //пришла команда стоп, вышли из цикла
    fprintf(stderr, "recorder: m_cmd_stop_thread: %d\n", m_cmd_stop_thread);
    //если цикл завершен, даем обратную связь
    m_thread_stopped = true;
}

template <class T>
void Audio_recorder<T>::stop(){
	if (m_thread_stopped == true) {
		fprintf(stderr, "recorder: ERROR! repeat stop()! Exit...\n");
		exit(0);
	}
	m_cmd_stop_thread = true; //даем команду  остановить цикл
	g_pDSCaptureBuffer->Stop(); //останавливает буфер, чтобы он больше не собирал данные.Если буфер не захватывается, метод не действует.
	
	size_t temp_count = 0;
	while (m_thread_stopped == false)	{
		// контролируем останов, с засыпанием между котролями на 10 мс
		std::this_thread::sleep_for(5ms);
		temp_count++;
		if (temp_count == 200){
			//если не дождались, пишем
			fprintf(stderr, "recorder: ERROR: m_thread_stopped: %d\n", m_thread_stopped);
			// и корректно выходим
			exit(1);
		}
	}
}