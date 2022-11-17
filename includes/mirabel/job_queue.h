#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//TODO maybe offer priorities for jobs?

//TODO downsizing the threadpool waits for unfinished jobs to end on the exiting threads: *dont't* make the gui wait for the downsizing
//TODO should auto cleanup threads by using cpp jthread, use thread detach?

//TODO fireandforget job item run mode: does not return anything and has to cleanup its own item after running

//TODO should have some more immediate parameters for the job to work with

typedef enum __attribute__((__packed__)) JOB_ITEM_STATE_E {
    JOB_ITEM_STATE_NONE = 0,
    JOB_ITEM_STATE_WAITING,
    JOB_ITEM_STATE_RUNNING,
    JOB_ITEM_STATE_ABORTED,
    JOB_ITEM_STATE_ERROR,
    JOB_ITEM_STATE_SUCCESS,
    JOB_ITEM_STATE_SIZE_MAX = UINT8_MAX,
} JOB_ITEM_STATE;

typedef struct job_item_s {
    char _padding[24];
} job_item;

typedef struct job_queue_s {
    char _padding[216];
} job_queue;

typedef JOB_ITEM_STATE job_work(job_item* ji, void** data);

void job_item_create(job_item* ji, void* data, job_work* work); // can be used on ANY job not waiting or running
JOB_ITEM_STATE job_item_get_state(job_item* ji);
void* job_item_get_data(job_item* ji); // only legal while job is not waiting or running
bool job_item_abort_requested(job_item* ji); // job work can check this periodically on itself to see if it was cancelled

// dont use multiple create/set_thread/destroy calls in parallel, for obvious reasons
void job_queue_create(job_queue* jq, uint32_t threads);
void job_queue_set_threads(job_queue* jq, uint32_t threads);
void job_queue_destroy(job_queue* jq);
void job_queue_item_push(job_queue* jq, job_item* ji);
bool job_queue_item_abort(job_queue* jq, job_item* ji); // returns true if the item was removed from the queue, false if it was already started

#ifdef __cplusplus
}
#endif
