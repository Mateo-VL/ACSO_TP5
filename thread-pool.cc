#include "thread-pool.h"
#include "Semaphore.h"
#include <iostream>
using namespace std;

void ThreadPool::dispatcher()
{
    while (true)
    {
        dtSem.wait();
        wSem.wait();
        {
            lock_guard<mutex> guard(thunkLock);
            if (kill)
            {
                return;
            }
            auto thunk = thunks.front();
            thunks.pop();

            for (auto &worker : wts)
            {
                lock_guard<mutex> workerGuard(worker.workLock);
                if (!worker.working)
                {
                    worker.thunk = thunk;
                    worker.working = true;
                    worker.workSem.signal();
                    break;
                }
            }
        }
    }
}

void ThreadPool::work(Worker *worker)
{
    while (true)
    {
        worker->workSem.wait();
        {
            lock_guard<mutex> workerGuard(worker->workLock);
            if (worker->kill)
            {
                return;
            }
        }

        // Execute the task
        worker->thunk();

        {
            lock_guard<mutex> workerGuard(worker->workLock);
            worker->working = false;
            wSem.signal();
            workercount--;
            if (workercount == 0)
            {
                wCV.notify_all();
            }
        }
    }
}

ThreadPool::ThreadPool(size_t numThreads)
    : wts(numThreads), wSem(numThreads)
{
    for (auto &worker : wts)
    {
        worker.th = thread([this, &worker]
                           { work(&worker); });
    }
    dt = thread([this]
                { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)> &thunk)
{
    {
        lock_guard<mutex> guard(thunkLock);
        thunks.push(thunk);
        workercount++;
    }
    dtSem.signal();
}

void ThreadPool::wait()
{
    unique_lock<mutex> lock(thunkLock);
    wCV.wait(lock, [this]
             { return workercount == 0; });
}

ThreadPool::~ThreadPool()
{
    wait();

    {
        lock_guard<mutex> guard(thunkLock);
        kill = true;
    }
    dtSem.signal();
    dt.join();

    for (auto &worker : wts)
    {
        {
            lock_guard<mutex> guard(worker.workLock);
            worker.kill = true;
            worker.workSem.signal();
        }
        worker.th.join();
    }
}
