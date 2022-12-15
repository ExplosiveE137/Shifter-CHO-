#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cmath>
#include <math.h>
#include <windows.graphics.h>
#include <fstream>
#include "dspl.hpp" // ���������� � FFT � ����������� ��������
#include <csignal> //��� ����������� ���������� ����� ������ �� ctrl+c
#include "AtomicQueue.hpp" // ��������� �������
#include "Audio_player_v2.hpp" // ����� �������������
#include "Audio_recorder_v2.hpp" // ����� ������������
#define SAMP_RATE 24000			//������� �������������
#define BITS_PER_SAMP 16		//��� �� �����
#define ATOMIC_QUEUE_TYPE short	// ��� ������ ������� (char ���� short)

AtomicQueue <ATOMIC_QUEUE_TYPE> sndMicBuffer;    //�����-������� ������ � ���������
AtomicQueue <ATOMIC_QUEUE_TYPE> sndDinBuffer;    //�����-������� ��� ��������������� �� �������

Audio_recorder	<ATOMIC_QUEUE_TYPE> recorder(&sndMicBuffer, BITS_PER_SAMP, SAMP_RATE);	// ����� �����������
Audio_player	<ATOMIC_QUEUE_TYPE> player(&sndDinBuffer, BITS_PER_SAMP, SAMP_RATE);	// ����� ��������������������
const int elementsInQueueInSec = player.getSampleRate() * player.getBitsPerSample() / 16;	//���-�� ��������� �������, ������� ����� �������������� � ������� ������� (������� ������������� * ���-�� �������� ������� �� �����)

bool testDirectSound_stop_flag = 0;	//���� ��������� testDirectSound(), ������� ����� ������ ���������� ����������

const int N = 2048; // ����� �������

void insert_delay_in_atomic(const int delay) {	// ��������
	char element = 0;	//������� ������� ������ � ������� X ���
	for (unsigned int i = 0; i < delay * elementsInQueueInSec; i++)
		sndDinBuffer.Push(element); // ������ � ������ �������� ������� �������� ��� �������� ��������
}

void stopSignalTestDirectSound(int signal) {	//���������� �������	
	if (signal == SIGINT) {
		testDirectSound_stop_flag = 1;
	}
}
// ����� ����������
void printInfo(time_t* work_time_start) {
	time_t total_time_sec = time(NULL) - *work_time_start;
	printf("work time: %lld m, %lld s\t\t", total_time_sec / 60, total_time_sec % 60);
	printf("mic size: %d\t", sndMicBuffer.Size());
	printf("audio size: %d\tdelay %.1f sec\n", sndDinBuffer.Size(), (float)sndDinBuffer.Size() / elementsInQueueInSec);
}
//���������� ������� �� main ����� ����� cpp ���� �� �����
void shift_up(double* inputData, double* outputData, const int count, const int freqshift);
void shift_down(double* inputData, double* outputData, const int count, const int freqshift);
int loadsignalbuffer(FILE* file, double* buffer, const int count);
void writesignal(const int count, double* signal, FILE* file);
//���������� ������� �� main ����� ����� cpp ���� �� �����
void secmainfile() {
	signal(SIGINT, stopSignalTestDirectSound);	//������������� ���������� ������� SignalHandler �� ��������� SIGIN (ctrl+C)
	ATOMIC_QUEUE_TYPE cur_sample;	//������� ���� �������� ���������� (������������� ��� ��� �����, �� ����� ������)
	int dalay_before_start = 0; //�������� ����� ������� ����� � ����������������
	recorder.start();	//������ ������
	player.start();		//������ ������
	double signal[N]; // ���� ����� ������ ������� ������
	double output[N]; // ���� ����� ������ �������� ������
	clock_t timer = clock();				// �������� ����� ��� ������ ���� � �������� �����
	time_t work_time_start = time(NULL);	//������ ����� ������ ������ DS
	FILE* fileInput = fopen(".\\math_model\\output.s16le", "rb"); // ��������� ���� � ������� ��������
	insert_delay_in_atomic(dalay_before_start); // ��������� �������� if == 0 {zaderjki net;}
	if ((fileInput == NULL)) // ��������� ������������� �����
	{
		fputs("������ �����", stderr);
		exit(1); // ����� ���, ��������� �����
	}
	while (testDirectSound_stop_flag == 0) {// ���� ���� == 0 �� ctrl+c �� ������ ������ ����� �������� ������
		int temp = loadsignalbuffer(fileInput, signal, N); // ������ �� ����� 2048 ������� ( ���� ����� ����� �� ��������� ������� ������ 1, ���� �������� ��� ���� �������� �� 0)
		if(temp!=0)//shift_up(signal, output, N, 330); // �������� �����
		shift_down(signal,output,N,-280); // �������� ����
		for (int i = 0; i < N; i++) // ��������� � ����� �������� ��� ��������� ������
			sndDinBuffer.Push((short)output[i]);
		if (clock() - timer > CLOCKS_PER_SEC * 1) {	//����� ���� ��� � � ���
			printInfo(&work_time_start);			//�������� ���������� ����������
			timer = clock(); //����� �������
		}
		Sleep(1);
	}
		fclose(fileInput); // ��������� ����
	recorder.stop(); // ��������� ������
	player.stop(); // ��������� ���������������
	
	exit(0); // ����� �� ���������
}
void secmain() {
	signal(SIGINT, stopSignalTestDirectSound);	//������������� ���������� ������� SignalHandler �� ��������� SIGIN (ctrl+C)
	ATOMIC_QUEUE_TYPE cur_sample;	//������� ���� �������� ���������� (������������� ��� ��� �����, �� ����� ������)
	int dalay_before_start = 0; //�������� ����� ������� ����� � ����������������
	recorder.start();	//������ ������
	player.start();		//������ ������
	double signal[N]; // ��� ����� ������� ������
	double output[N]; // ��� ����� �������� ������
	clock_t timer = clock();				// �������� ����� ��� ������ ���� � �������� �����
	time_t work_time_start = time(NULL);	//������ ����� ������ ������ DS

	insert_delay_in_atomic(dalay_before_start); // ��������� ��������
	FILE* fileout = fopen(".\\math_model\\output.s16le", "wb"); // ������ ����� ���  ������ ��������� ������ ����� ����� �������� ������� �������� ���� � ����� ��� ������������
	while (testDirectSound_stop_flag == 0) {//���� �� ������� ������ ctrl+c
		if (sndMicBuffer.Size() > N) // ���� � ������� ��������� ������� ������ ��� 2048 �� �������� ��������
		{
			for (int i = 0; i < N; i++) { // ������ ������
				sndMicBuffer.Pull(cur_sample); // ����� �� �������(�������) �����
				signal[i] = (double)cur_sample; // ������ ����� � ������� ������ ������
			}
			writesignal(N, signal, fileout); // ������� ������ ��������� ������ ����� ����� �������� ������� �������� ���� � ����� ��� ������������
			// ��������
			//shift_up(signal, output, N, 500);
			shift_down(signal, output, N, -300);
			// �������� ���������
			for (int i = 0; i < N; i++)
			{
				sndDinBuffer.Push((short)output[i]);//���������� ����� �� �������
			}
				
			
		}
		if (clock() - timer > CLOCKS_PER_SEC * 1) {	//����� ���� ��� � � ���
			printInfo(&work_time_start);			//�������� ���������� ����������
			timer = clock(); //����� �������
		}
		Sleep(1);
	}
	fclose(fileout);
	recorder.stop();
	player.stop();

	exit(0);
}