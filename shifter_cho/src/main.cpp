#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cmath>
#include <math.h>
#include <fstream>
#include "dspl.hpp" // библиотека с FFT и комплексным массивом
#include <csignal> //для корректного завершения через сигнал от ctrl+c
#include "AtomicQueue.hpp" // Атомарные очереди
#include "Audio_player_v2.hpp" // Аудио проигрователь
#include "Audio_recorder_v2.hpp" // Аудио записыватель
//входной сигна- спектр
std::ofstream InputSignalSpecRe((char*)".\\math_model\\RE.txt"); // открываем поток для Re
std::ofstream InputSignalSpecIm((char*)".\\math_model\\Im.txt"); // открываем поток для Im
// выходной сигнал - спектр
std::ofstream OutputSignalSpecRe((char*)".\\math_model\\ShiftRe.txt"); // открываем поток для Re
std::ofstream OutputSignalSpecIm((char*)".\\math_model\\ShiftIM.txt"); // открываем поток для Im

std::ofstream OutputSignal((char*)".\\math_model\\OutputData.txt"); // открываем поток Выходного сигнала

std::ofstream InputSignal((char*)".\\math_model\\InputData.txt"); // открываем поток Входного сигнала

#define M_2PI       6.283185307179586476925286766559 // Наша ПИ
using namespace std;
const int N = 2048; // количество отчетов
const int fd = 24'000; // Частота дискретизации 
complex_t complexSignal[N]; // Комлпексный массив для FFT

void upload_to_file(complex_t* data, const int count, ofstream& FileRe,ofstream& FileIm) { // запись в файл мнимой и действительной части для построения графиков в маткаде для сверки
	// производим запись
	for (int i = 0; i < count; i++) {
		FileRe << RE(data[i]) << '\n';
		FileIm << IM(data[i]) << '\n';
	}
}
void upload_to_file(double* data, const int count, ofstream& FileRe) {
	for (int i = 0; i < count; i++) {
		FileRe << data[i] << '\n';
	}
}
void shift_up(double* inputData, double* outputData,const int count,const int freqshift) {
	// Смещение вверх
	fft_t pfft = { 0 };       /* FFT object (fill zeros)  */ // FFT объект заполенный нулями
	fft_create(&pfft, count);
	/* Fill FFT structure                               */
	fft(inputData, count, &pfft, complexSignal);
	upload_to_file(complexSignal, N, InputSignalSpecRe, InputSignalSpecIm); // вывод спектра исходного
	fft_free(&pfft);        /* Clear fft_t object       */
	complex_t ShiftArr[N] = { 0 }; 
	double freq_otch = (double)fd / N; // Частота одного отчета
	int shift = ceil(freqshift / freq_otch); // Количество отчетов на которое смещаем
	int last_otch = ceil(4000 / freq_otch); // последний отчет в нашей полосе частот 0.3-3.4 кГц
	for (int i = 0; i < last_otch; i++) { // смещение
		RE(ShiftArr[i + shift]) = RE(complexSignal[i]);
		IM(ShiftArr[i + shift]) = IM(complexSignal[i]);
		RE(ShiftArr[N - i - shift]) = RE(complexSignal[i]);
		IM(ShiftArr[N - i - shift]) = IM(complexSignal[i]);
	}
	ifft_cmplx(ShiftArr, N, &pfft, complexSignal); // обратное преобразование фурье
	for (int i = 0; i < N; i++) { // вывод смещенного сигнала в выходной массив
		outputData[i] = RE(complexSignal[i]);
	}
	upload_to_file(inputData, N, InputSignal); // Вывод исходного
	upload_to_file(ShiftArr, N, OutputSignalSpecRe, OutputSignalSpecIm); // Вывод спекта смещенного сигнала
	upload_to_file(outputData, N, OutputSignal); // Вывод смещенного сигнала
}
void shift_down(double* inputData, double* outputData, const int count, const int freqshift) {
	// Смещение вверх
	fft_t pfft = { 0 };       /* FFT object (fill zeros)  */ // FFT объект заполенный нулями
	fft_create(&pfft, count);
	/* Fill FFT structure                                */
	fft(inputData, count, &pfft, complexSignal);
	upload_to_file(complexSignal, N, InputSignalSpecRe, InputSignalSpecIm); // вывод спектра исходного
	fft_free(&pfft);        /* Clear fft_t object       */
	complex_t ShiftArr[N] = { 0 };
	double freq_otch = (double)fd / N; // Частота одного отчета
	int shift = ceil(freqshift / (double)freq_otch); // Кол-во отчетов на которое надо сместить
	int last_otch = ceil((double)4000 / (double)freq_otch); //  последний отчет нашего канала 0.3-3.4кГц
	for (int i = abs((1-last_otch)); i > abs(shift); i--) { // смещение
		RE(ShiftArr[i + shift]) = RE(complexSignal[i]);
		IM(ShiftArr[i + shift]) = IM(complexSignal[i]);
		RE(ShiftArr[N - i - shift]) = RE(complexSignal[i]);
		IM(ShiftArr[N - i - shift]) = IM(complexSignal[i]);
	}
	ifft_cmplx(ShiftArr, N, &pfft, complexSignal); //  обратное преобразование фурье
	for (int i = 0; i < N; i++) {
		outputData[i] = RE(complexSignal[i]); // вывод нашего сигнала
	}
	upload_to_file(inputData, N, InputSignal); // Вывод исходного
	upload_to_file(complexSignal, N, InputSignalSpecRe, InputSignalSpecIm); // вывод спектра
	upload_to_file(ShiftArr, N, OutputSignalSpecRe, OutputSignalSpecIm); // Вывод спекта смещенного сигнала
	upload_to_file(outputData, N, OutputSignal); // Вывод смещенного сигнала
}
int loadsignalbuffer(FILE * file, double* buffer,const int count) { // Функция забора отчетов из звукового файла
	for (int i = 0; i < count; i++)
	{
		short shortbuff; // 2-байтовый элемент
		size_t countelement = fread(&shortbuff, 2, 1, file); // читаем элементы 1 элемент = 2 байта
		if (countelement == 0) // проверка конца файла
		{
			return 0; // файл кончился
			break;
		}
		buffer[i] = shortbuff; // в буфер пихаем наши отчеты
	}
	return 1;
}
void writesignal(const int count, double* signal, FILE* file) { // запись сигнала в файл
	for (int i = 0; i < N; i++)
	{
		short shortbuff = signal[i]; // 2-байтовый элемент берет себе 1 отчет
		size_t countelemnt = fwrite(&shortbuff, 2, 1, file); // запихиваем его в файл
		if (countelemnt == 0) // если в файл записывать нечего завершаем функцию
		{
			break;
		}
	}
}
void secmain(); // функция вывода звука на динамики с микрофона
void secmainfile(); // функция вывода звука на динамики из файла
int main() {
	void* handle;       /* DSPL handle              */
	handle = dspl_load();   /* Load libdspl             */
	//secmainfile(); // из файла на динамики
	secmain(); // real time


	// проверка роботоспособности программы с файла в файл
	//double signal[N];
	//double output[N];
	//FILE* fileInput = fopen(".\\math_model\\output.s16le", "rb");
	//FILE* FileOut = fopen(".\\math_model\\end.s16le", "wb");
	//if ((fileInput == NULL) && (FileOut == NULL))
	//{
	//	fputs("Ошибка файла", stderr);
	//	exit(1);
	//}
	//while (1) {
	//	int temp = loadsignalbuffer(fileInput, signal, N);
	//	//shift_up(signal,output,N,500);
	//	//shift_down(signal,output,N,-300);
	//	writesignal(N, output, FileOut);
	//	if (temp == 0)
	//	{
	//		exit(0);
	//	}
	//}
	//fclose(FileOut);
	
	////конец проверки
	InputSignal.close();
	OutputSignal.close();
	OutputSignalSpecIm.close();
	OutputSignalSpecRe.close();
	InputSignalSpecRe.close();
	InputSignalSpecIm.close();
	dspl_free(handle); // Clear DSPL handle        
	return 0;
}