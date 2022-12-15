#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cmath>
#include <math.h>
#include <windows.graphics.h>
#include <fstream>
#include "dspl.hpp" // библиотека с FFT и комплексным массивом
#include <csignal> //для корректного завершения через сигнал от ctrl+c
#include "AtomicQueue.hpp" // Атомарные очереди
#include "Audio_player_v2.hpp" // Аудио проигрователь
#include "Audio_recorder_v2.hpp" // Аудио записыватель
#define SAMP_RATE 24000			//частота дискретизации
#define BITS_PER_SAMP 16		//бит на семпл
#define ATOMIC_QUEUE_TYPE short	// тип атомик очереди (char либо short)

AtomicQueue <ATOMIC_QUEUE_TYPE> sndMicBuffer;    //буфер-очередь записи с микрофона
AtomicQueue <ATOMIC_QUEUE_TYPE> sndDinBuffer;    //буфер-очередь для воспроизведения на динамик

Audio_recorder	<ATOMIC_QUEUE_TYPE> recorder(&sndMicBuffer, BITS_PER_SAMP, SAMP_RATE);	// класс звукозаписи
Audio_player	<ATOMIC_QUEUE_TYPE> player(&sndDinBuffer, BITS_PER_SAMP, SAMP_RATE);	// класс звуковоспроизведения
const int elementsInQueueInSec = player.getSampleRate() * player.getBitsPerSample() / 16;	//кол-во элементов очереди, которые будут воспроизведены в течении секунды (частота дискретизации * кол-во элеметов атомика на семпл)

bool testDirectSound_stop_flag = 0;	//флаг остановки testDirectSound(), измение флага произв обработчик прерывания

const int N = 2048; // число отчетов

void insert_delay_in_atomic(const int delay) {	// Задержка
	char element = 0;	//Элемент который кладем в очередь X раз
	for (unsigned int i = 0; i < delay * elementsInQueueInSec; i++)
		sndDinBuffer.Push(element); // Кладем в буффер динамика нулевые элементы для создания задержки
}

void stopSignalTestDirectSound(int signal) {	//обработчик сигнала	
	if (signal == SIGINT) {
		testDirectSound_stop_flag = 1;
	}
}
// вывод информации
void printInfo(time_t* work_time_start) {
	time_t total_time_sec = time(NULL) - *work_time_start;
	printf("work time: %lld m, %lld s\t\t", total_time_sec / 60, total_time_sec % 60);
	printf("mic size: %d\t", sndMicBuffer.Size());
	printf("audio size: %d\tdelay %.1f sec\n", sndDinBuffer.Size(), (float)sndDinBuffer.Size() / elementsInQueueInSec);
}
//объявление функций из main чтобы новый cpp файл их видел
void shift_up(double* inputData, double* outputData, const int count, const int freqshift);
void shift_down(double* inputData, double* outputData, const int count, const int freqshift);
int loadsignalbuffer(FILE* file, double* buffer, const int count);
void writesignal(const int count, double* signal, FILE* file);
//объявление функций из main чтобы новый cpp файл их видел
void secmainfile() {
	signal(SIGINT, stopSignalTestDirectSound);	//устанавливает обработчик сигнала SignalHandler на обработку SIGIN (ctrl+C)
	ATOMIC_QUEUE_TYPE cur_sample;	//текущий байт звуковой информации (необязательно что это семпл, мб часть семпла)
	int dalay_before_start = 0; //задержка между записью звука и воспроизведением
	recorder.start();	//запуск записи
	player.start();		//запуск плеера
	double signal[N]; // Сюда будем класть входной сигнал
	double output[N]; // Сюда будем класть выходной сигнал
	clock_t timer = clock();				// засекает время для вывода инфы в основном цикле
	time_t work_time_start = time(NULL);	//хранит время начала работы DS
	FILE* fileInput = fopen(".\\math_model\\output.s16le", "rb"); // открываем файл с входным сигналом
	insert_delay_in_atomic(dalay_before_start); // вставляем задержку if == 0 {zaderjki net;}
	if ((fileInput == NULL)) // проверяем существование файла
	{
		fputs("Ошибка файла", stderr);
		exit(1); // файла нет, закрываем прогу
	}
	while (testDirectSound_stop_flag == 0) {// если флаг == 0 то ctrl+c не нажали значит можно воровать семплы
		int temp = loadsignalbuffer(fileInput, signal, N); // воруем из файла 2048 отчетов ( если конец файла не достигнут функция вернет 1, если достигли или файл сломался то 0)
		if(temp!=0)//shift_up(signal, output, N, 330); // смещение вверх
		shift_down(signal,output,N,-280); // смещение вниз
		for (int i = 0; i < N; i++) // вставляем в поток динамика наш смещенный сигнал
			sndDinBuffer.Push((short)output[i]);
		if (clock() - timer > CLOCKS_PER_SEC * 1) {	//вывод инфы раз в Х сек
			printInfo(&work_time_start);			//печатает отладочную информацию
			timer = clock(); //сброс таймера
		}
		Sleep(1);
	}
		fclose(fileInput); // закрываем файл
	recorder.stop(); // остановка записи
	player.stop(); // остоновка воспроизведения
	
	exit(0); // выход из программы
}
void secmain() {
	signal(SIGINT, stopSignalTestDirectSound);	//устанавливает обработчик сигнала SignalHandler на обработку SIGIN (ctrl+C)
	ATOMIC_QUEUE_TYPE cur_sample;	//текущий байт звуковой информации (необязательно что это семпл, мб часть семпла)
	int dalay_before_start = 0; //задержка между записью звука и воспроизведением
	recorder.start();	//запуск записи
	player.start();		//запуск плеера
	double signal[N]; // тут будет входной сигнал
	double output[N]; // тут будет выходной сигнал
	clock_t timer = clock();				// засекает время для вывода инфы в основном цикле
	time_t work_time_start = time(NULL);	//хранит время начала работы DS

	insert_delay_in_atomic(dalay_before_start); // вставляем задержку
	FILE* fileout = fopen(".\\math_model\\output.s16le", "wb"); // открыл поток для  записи основного потока звука чтобы записать длинный звуковой файл и потом его обрабатывать
	while (testDirectSound_stop_flag == 0) {//Пока не получен сигнал ctrl+c
		if (sndMicBuffer.Size() > N) // если в очереди микрофона семплов больше чем 2048 то начинаем воровать
		{
			for (int i = 0; i < N; i++) { // воруем семплы
				sndMicBuffer.Pull(cur_sample); // берем из очереди(сначала) семпл
				signal[i] = (double)cur_sample; // пихаем семпл в входной массив даблов
			}
			writesignal(N, signal, fileout); // добавил запись основного потока звука чтобы записать длинный звуковой файл и потом его обрабатывать
			// смещение
			//shift_up(signal, output, N, 500);
			shift_down(signal, output, N, -300);
			// смещение закончено
			for (int i = 0; i < N; i++)
			{
				sndDinBuffer.Push((short)output[i]);//закидываем семлп на динамик
			}
				
			
		}
		if (clock() - timer > CLOCKS_PER_SEC * 1) {	//вывод инфы раз в Х сек
			printInfo(&work_time_start);			//печатает отладочную информацию
			timer = clock(); //сброс таймера
		}
		Sleep(1);
	}
	fclose(fileout);
	recorder.stop();
	player.stop();

	exit(0);
}