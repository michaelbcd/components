/*
 * Copyright (c) 2018-2019, Michael Wang. All rights reserved.
 *
 *******************************************************************************
 * @file   demo_readwritelock.cpp
 * @brief  demo for readwritelock
 * compile: g++ -o readwritelock -std=c++11 -I.. demo_readwritelock.cpp -lpthread
 *******************************************************************************
 */

#include <iostream>       // std::cout
#include <chrono>         // std::chrono::milliseconds
#include <thread>         // std::thread
#include "readwritelock.h"



int RacingData = 0;

void writeData(Semaphore* sem)
{
    RWWriteLock writelck(*sem);

    for(int i = 1; i < 10; i++)
    {
        std::cout << "Thread writer try to write the data in loop: " << i << std::endl;
        writelck.writeOccupy();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        RacingData = i;

        std::cout << "Thread writer has written the data: " << RacingData << std::endl;
        writelck.writeRelease();
    }
}

void readData(Semaphore* sem, int id)
{
    RWReadLock readlck(*sem);
    int data = 0;

    for(int i = 0; i < 10; i++)
    {
        std::cout << "Thread id: " << id << " try to read the data in loop: " << i << std::endl;
        readlck.readOccupy();
        data = RacingData;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        std::cout << "Thread id: " << id << " has read the data: " << data << std::endl;
        // for non-loop usage, readRelease can be omitted as it will be called in dctor
        readlck.readRelease();
    }
}

int main()
{
    std::cout << "Starting main.\n";
    // MAX_COUNT is larger than the count of readers.
    Semaphore sem(MAX_COUNT);

    const int ThdNum = 2;
    std::thread readThd[ThdNum];
    for(int i = 0; i < ThdNum; i++)
    {
        readThd[i] = std::thread(readData, &sem, i);
    }

    std::thread writeThd;
    writeThd = std::thread(writeData, &sem);

    std::cout << "main waiting to join the threads.\n";
    //std::this_thread::sleep_for(std::chrono::seconds(10));
    for(auto& th : readThd)
    {
        th.join();
    }
    writeThd.join();
    std::cout << "Exiting main.\n";
}


