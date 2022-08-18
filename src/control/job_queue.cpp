#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "mirabel/job_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct job_item_impl {
    void* data;
    JOB_ITEM_STATE (*work)(job_item* ji, void** data);
    std::atomic<JOB_ITEM_STATE> state;
    std::atomic<bool> abort;
};

struct job_thread {
    std::thread runner;
    std::atomic<bool> exit;
};

//TODO this job queue isnt very efficient especially with many threads
struct job_queue_impl {
    std::mutex m;
    std::deque<job_item_impl*> q;
    std::condition_variable cv;
    std::vector<job_thread*> threads;
};

static_assert(sizeof(job_item) >= sizeof(job_item_impl), "job_item impl size missmatch");

static_assert(sizeof(job_queue) >= sizeof(job_queue_impl), "job_queue impl size missmatch");

void job_item_create(job_item* ji, void* data, JOB_ITEM_STATE (*work)(job_item* ji, void** data))
{
    job_item_impl* jii = (job_item_impl*)ji;
    new(jii) job_item_impl;
    jii->data = data;
    jii->work = work;
    jii->state = JOB_ITEM_STATE_NONE;
    jii->abort = false;
}

JOB_ITEM_STATE job_item_get_state(job_item* ji)
{
    job_item_impl* jii = (job_item_impl*)ji;
    return jii->state;
}

void* job_item_get_data(job_item* ji)
{
    job_item_impl* jii = (job_item_impl*)ji;
    return jii->data;
}

bool job_item_abort_requested(job_item* ji)
{
    job_item_impl* jii = (job_item_impl*)ji;
    return jii->abort;
}

// impl hidden
void job_runner(job_thread* jt, job_queue_impl* jqi)
{
    while (true) {
        job_item_impl* next_job = NULL;
        {
            // under lock
            std::unique_lock<std::mutex> lock(jqi->m);
            if (jqi->q.size() == 0) {
                jqi->cv.wait(lock);
            }
            if (jt->exit == true) {
                return;
            }
            while (jqi->q.size() > 0 && next_job == NULL) {
                next_job = jqi->q.front();
                jqi->q.pop_front();
                if (next_job->abort == true) {
                    next_job->state = JOB_ITEM_STATE_ABORTED;
                    next_job = NULL;
                }
            }
        }
        // not holding the lock anymore
        if (next_job != NULL) {
            next_job->state = JOB_ITEM_STATE_RUNNING;
            JOB_ITEM_STATE job_finish_state = next_job->work((job_item*)next_job, &next_job->data);
            if (job_finish_state < JOB_ITEM_STATE_ABORTED) {
                job_finish_state = JOB_ITEM_STATE_ERROR;
            }
            next_job->state = job_finish_state;
        }
    }
}

void job_queue_create(job_queue* jq, uint32_t threads)
{
    job_queue_impl* jqi = (job_queue_impl*)jq;
    new(jqi) job_queue_impl();
    std::unique_lock<std::mutex> lock(jqi->m);
    for (uint32_t tc = 0; tc < threads; tc++) {
        job_thread* jt = new job_thread;
        jt->exit = false;
        jt->runner = std::thread(job_runner, jt, jqi);
        jqi->threads.emplace_back(jt);
    }
}

void job_queue_set_threads(job_queue* jq, uint32_t threads)
{
    job_queue_impl* jqi = (job_queue_impl*)jq;
    jqi->m.lock();
    if (threads == jqi->threads.size()) {
        jqi->m.unlock();
        return;
    } else if (threads > jqi->threads.size()) {
        // add more threads
        for (int64_t tc = jqi->threads.size(); tc < threads; tc++) {
            job_thread* jt = new job_thread;
            jt->exit = false;
            jt->runner = std::thread(job_runner, jt, jqi);
            jqi->threads.emplace_back(jt);
        }
    } else {
        // remove threads
        for (int64_t tc = jqi->threads.size() - 1; tc >= threads; tc--) {
            jqi->threads[tc]->exit = true;
        }
        jqi->cv.notify_all();
        //TODO lock reaquire after join really needed?
        for (int64_t tc = jqi->threads.size() - 1; tc >= threads; tc--) {
            jqi->m.unlock();
            jqi->threads[tc]->runner.join();
            jqi->m.lock();
            delete jqi->threads.back();
            jqi->threads.pop_back();
        }
    }
    jqi->m.unlock();
}

void job_queue_destroy(job_queue* jq)
{
    // dont need a lock since we're destroying the queue anyway
    job_queue_impl* jqi = (job_queue_impl*)jq;
    job_queue_set_threads(jq, 0); // let outstanding jobs finish, now we have no more threads
    while (jqi->q.size() > 0) {
        jqi->q.front()->state = JOB_ITEM_STATE_ABORTED;
        jqi->q.pop_front();
    }
    jqi->~job_queue_impl();
}

void job_queue_item_push(job_queue* jq, job_item* ji)
{
    job_queue_impl* jqi = (job_queue_impl*)jq;
    std::unique_lock<std::mutex> lock(jqi->m);
    jqi->q.emplace_back((job_item_impl*)ji);
    ((job_item_impl*)ji)->state = JOB_ITEM_STATE_WAITING;
    jqi->cv.notify_all();
}

bool job_queue_item_abort(job_queue* jq, job_item* ji)
{
    ((job_item_impl*)ji)->abort = true;
    job_queue_impl* jqi = (job_queue_impl*)jq;
    {
        std::unique_lock<std::mutex> lock(jqi->m);
        for (std::deque<job_item_impl*>::iterator it = jqi->q.begin(); it != jqi->q.end(); ++it) {
            if (*it == (job_item_impl*)ji) {
                (*it)->state = JOB_ITEM_STATE_ABORTED;
                jqi->q.erase(it);
                return true;
            }
        }
    }
    return false;
}

#ifdef __cplusplus
}
#endif
