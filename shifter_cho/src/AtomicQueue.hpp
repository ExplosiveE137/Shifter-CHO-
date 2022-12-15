#pragma once
#include <atomic>

template <typename T>   //Это не объявление класса, а шаблон
class AtomicQueue
{
public:
    AtomicQueue();
    bool Push(const T &val);    // Запись в конец очереди
    bool Pull(T &val);          // чтение из начала очереди
    unsigned int Size();

    unsigned int getMaxLength();//new! Получить LENGTH
    void reset();               //new! сброс очереди в начальное состояние
private:
    static const unsigned int LENGTH = 4000000; //Вот тут был мой косяк (размер очереди был слишком мал)

    std::atomic<unsigned int> read_ind;     //объявляем атомарные переменные
    std::atomic<unsigned int> write_ind;
    std::atomic<unsigned int> size;     
    T buffer[LENGTH];                       //массив очереди пользовательского типа
};

typedef unsigned char byte;
template class AtomicQueue<char>;   //явно создаем экземпляр шаблона класса
template class AtomicQueue<short>;  //явно создаем экземпляр шаблона класса
template class AtomicQueue<byte>;   //явно создаем экземпляр шаблона класса