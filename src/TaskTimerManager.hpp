#ifndef _TASK_TIMER_MANANGER_H_
#define _TASK_TIMER_MANANGER_H_
#include <map>
#include <iostream>
#include <time.h>
#include <tr1/memory>
#include <tr1/functional>
#include <stdint.h>
#include <stdio.h>

template <class T>
struct TaskTimer {
    TaskTimer():expiration(0), task_id(0),callback(0) {
    };
    int expiration;
    int64_t task_id;
    T user_data;
    void (*callback) (const TaskTimer* data);
};

template <class T>
class TaskTimerManager {
    typedef std::multimap< int, TaskTimer<T> > CallbackMap;

    typedef typename CallbackMap::iterator CallbackIter;
private:
    CallbackMap callbacks;
    pthread_mutex_t callbacks_mutex;

    TaskTimerManager() {
        pthread_mutex_init(&callbacks_mutex, NULL);
    }

    TaskTimerManager(const TaskTimerManager&);
    TaskTimerManager& operator=(const TaskTimerManager&); //non-copyable.
public:
    void* execute(void *data);
    void add(TaskTimer<T> &timer);
    void cancel(TaskTimer<T> &timer);
    static TaskTimerManager& getInstance();
};

template <class T>
TaskTimerManager<T>& TaskTimerManager<T>::getInstance() {
    static TaskTimerManager p;
    return p;
};

template <class T>
void* TaskTimerManager<T>::execute(void *arg) 
{
    //lint -e(716)
    while(true) {
        pthread_mutex_lock(&callbacks_mutex);
        int now = time(NULL);
        CallbackIter it_end = callbacks.upper_bound(now);
        for(CallbackIter it = callbacks.begin(); it != it_end; ++it) {
            TaskTimer<T>& timer = it->second;
            timer.callback(&timer);
        }
        //erase callbacks that were called.
        callbacks.erase(callbacks.begin(), it_end);
        pthread_mutex_unlock(&callbacks_mutex);
        (void)sleep(1);
    }
    return NULL;
}

template <class T>
void TaskTimerManager<T>::add(TaskTimer<T> &timer)
{
    pthread_mutex_lock(&callbacks_mutex);
    callbacks.insert(std::make_pair(timer.expiration, timer));
    pthread_mutex_unlock(&callbacks_mutex);
}

template <class T>
void TaskTimerManager<T>::cancel(TaskTimer<T> &timer)
{
    pthread_mutex_lock(&callbacks_mutex);
    std::pair<CallbackIter, CallbackIter> range;
    range = callbacks.equal_range(timer.expiration);
    CallbackIter it;
    for (it = range.first; it != range.second; ++it) {
        TaskTimer<T>& t = it->second;
        if (t.task_id == timer.task_id) {
            break;
        }
    }
    if(it != range.second) {
        callbacks.erase(it);
    }
    pthread_mutex_unlock(&callbacks_mutex);
}

#endif
