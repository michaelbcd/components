/*
 * Copyright (c) 2018-2019, Michael Wang. All rights reserved.
 *
 *******************************************************************************
 * @file   ReadWriteLock.h
 * @brief  complete fair read-write lock; reader and writer are fair to get
 * 		   the channce to run
 * Implemention under c++11.
 *******************************************************************************
 */

#ifndef READWRITELOCK_H
#define READWRITELOCK_H

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

typedef int                     int32;
typedef unsigned int            uint32;


// define a big number larger than the count of readers
enum {
    MAX_COUNT = 1000
};


class Semaphore
{
public:
    Semaphore(uint32 count = MAX_COUNT)
        : limit_(count), count_(count)
    {}

    inline void occupy(uint32 count)
    {
        std::unique_lock<std::mutex> lock(mtx_);

        while(count_ < count)
        {
            cv_.wait(lock);
        }
        count_ -= count;
    }

    inline void release(uint32 count)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if(count_ == limit_)
        {
            return;
        }

        count_ += count;
        if(count_ > limit_)
        {
            count_ = limit_;
        }
        cv_.notify_all();
    }

    inline void occupyAll()
    {
        std::unique_lock<std::mutex> lock(mtx_);

        while(count_ < limit_)
        {
            cv_.wait(lock);
        }
        count_ = 0;
    }

    inline void releaseAll()
    {
        std::unique_lock<std::mutex> lock(mtx_);

        count_ = limit_;
        cv_.notify_all();
    }

    inline bool occupyWait(uint32 count, uint32 ms)
    {
        std::chrono::milliseconds msNum(ms);
        std::unique_lock<std::mutex> lock(mtx_);

        while(count_ < count)
        {
            cv_.wait_for(lock, msNum);
        }
        if(count_ >= count)
        {
            count_ -= count;
            return true;
        }
        return false;
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    const uint32 limit_;
    volatile uint32 count_;
};

class RWReadLock
{
public:
    RWReadLock(Semaphore &sem)
        : sem_(sem), count_(0)
    {}

    ~RWReadLock()
    {
        if(count_ > 0)
        {
            sem_.release(count_);
            count_ = 0;
        }
    }

    inline void readOccupy()
    {
        sem_.occupy(1);
        count_.fetch_add(1);
    }

    inline void readRelease()
    {
        int32 cn = 0;
        while((cn = count_.load()) >= 1)
        {
            if(count_.compare_exchange_weak(cn, cn - 1))
            {
                sem_.release(1);
                break;
            }
        }
    }

private:
    Semaphore &sem_;
    std::atomic<int32> count_;
};

class RWWriteLock
{
public:
    RWWriteLock(Semaphore &sem)
        : sem_(sem), bOccupied(false)
    {}

    ~RWWriteLock()
    {
        writeRelease();
    }

    inline void writeOccupy()
    {
        sem_.occupyAll();
        bOccupied = true;
    }

    inline void writeRelease()
    {
        if(bOccupied.exchange(false))
        {
            sem_.releaseAll();
        }
    }

private:
    Semaphore &sem_;
    std::atomic<bool> bOccupied;
};

#endif //READWRITELOCK_H

