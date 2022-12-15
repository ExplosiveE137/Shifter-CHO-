#include "AtomicQueue.hpp"
//#include "stdafx.h"
template< typename T >
AtomicQueue<T>::AtomicQueue()
{
    reset(); //возврат в начальное состояние
}

template< typename T >
void AtomicQueue<T>::reset() {
    read_ind.store(0);  // Выполняем сброс очереди
    write_ind.store(0); // Выполняем сброс очереди
    size.store(0);      // Выполняем сброс очереди
}

template< typename T >
bool AtomicQueue<T>::Push(const T &val)         // Запись в конец очереди
{
    unsigned int ind = write_ind.load();        //читаем текущий индекс последнего элемента (он указывает на следующий пустой элемент)
    unsigned int next_ind = (ind + 1) % LENGTH; //увеличиваем на единицу по модулю

    // если новый индекс не совпадает с индексом первого элемента очереди
    if(next_ind != read_ind.load())
    {
        buffer[ind] = val;          //пишем значение переданное по ссылке в конец
        size++;                     //увеличиваем размер очереди
        write_ind.store(next_ind);  //и только после записи модифицируем индекс
        return true;
    }
    else    //иначе буфер заполнен, свободного места нет
    {
        return false;
    }
}

template< typename T >
bool AtomicQueue<T>::Pull(T &val)       //чтение из начала очереди
{
    unsigned int ind = read_ind.load(); //читаем текущий индекс первого элемента очереди

    if(ind != write_ind.load())         //если индекс начала не совпадает с индексом последнего элемента
    {
        val = buffer[ind];                          // читаем элемент из начала и передаем его по ссылке
        size--;                                     //уменьшаем размер очереди
        unsigned int next_ind = (ind + 1) % LENGTH; //смещаем указатель на начало на один элемент
        read_ind.store(next_ind);                   //модифицируем индекс перврго элемента
        return true;
    }
    else     //иначе буфер пуст
    {
        return false;
    }
}

template< typename T >
unsigned int AtomicQueue<T>::Size()
{
    return size.load();
}

template< typename T >
unsigned int AtomicQueue<T>::getMaxLength(){
    return LENGTH;
}
