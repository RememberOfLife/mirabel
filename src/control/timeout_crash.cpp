#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "meta_gui/meta_gui.hpp"

#include "control/client.hpp"
#include "control/event_queue.h"
#include "control/event.h"

#include "control/timeout_crash.hpp"

namespace Control {

    void TimeoutCrash::timeout_info::send_heartbeat()
    {
        f_event_any es;
        f_event_create_heartbeat(&es, EVENT_TYPE_HEARTBEAT, id, 0);
        f_event_queue_push(q, &es);
    }

    void TimeoutCrash::timeout_info::pre_quit(uint32_t timeout)
    {
        f_event_any es;
        f_event_create_heartbeat(&es, EVENT_TYPE_HEARTBEAT_PREQUIT, id, timeout);
        f_event_queue_push(q, &es);
    }

    TimeoutCrash::timeout_item::timeout_item():
        id(0),
        name(NULL),
        q(NULL),
        quit(false)
    {}

    TimeoutCrash::timeout_item::timeout_item(f_event_queue* target_queue, uint32_t new_id, const char* new_name, int new_initial_delay, int new_timeout_ms):
        id(new_id),
        timeout_ms(new_timeout_ms),
        last_heartbeat_age(-new_initial_delay),
        heartbeat_answered(false),
        q(target_queue),
        quit(false)
    {
        name = (char*)malloc(strlen(new_name)+1);
        strcpy(name, new_name);
    }

    TimeoutCrash::TimeoutCrash()
    {
        f_event_queue_create(&inbox);
    }

    TimeoutCrash::~TimeoutCrash()
    {
        f_event_queue_destroy(&inbox);
    }
    
    void TimeoutCrash::loop()
    {
        bool quit = false;
        while (!quit) {

            // get the time to sleep until we need to check the earliest heartbeat again
            m.lock();
            int heartbeat_deadline = INT_MAX; // in ms as everything here is
            std::unordered_map<uint32_t, timeout_item>::iterator map_iter = timeout_items.begin();
            while (map_iter != timeout_items.end()) {
                timeout_item& tii = map_iter->second;
                int item_deadline = tii.timeout_ms - tii.last_heartbeat_age;
                if (item_deadline < heartbeat_deadline) {
                    heartbeat_deadline = item_deadline;
                }
                map_iter++;
            }
            m.unlock();

            std::chrono::steady_clock::time_point sleep_start = std::chrono::steady_clock::now();
            f_event_any e;
            f_event_queue_pop(&inbox, &e, heartbeat_deadline);
            std::chrono::steady_clock::time_point sleep_stop = std::chrono::steady_clock::now();
            int real_sleep_time = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_stop-sleep_start).count();
            // upon wakeup, check for heartbeat responses
            m.lock();
            while (e.base.type != EVENT_TYPE_NULL) {
                // process event e
                switch (e.base.type) {
                    case EVENT_TYPE_EXIT: {
                        quit = true;
                        MetaGui::log("#I timeout_crash: exit\n"); //TODO may only exit once all timeout_items are cleaned up, lest they use our deleted inbox
                        break;
                    } break;
                    case EVENT_TYPE_HEARTBEAT: {
                        auto ce = event_cast<f_event_heartbeat>(e);
                        if (timeout_item_exists(ce.id)) {
                            if (!timeout_items[ce.id].quit) {
                                timeout_items[ce.id].heartbeat_answered = true;
                            } else {
                                MetaGui::logf("#W timeout_crash: received heartbeat for prequit timeout item #%d\n", ce.id);
                            }
                        } else {
                            MetaGui::logf("#E timeout_crash: received heartbeat for unknown timeout item #%d\n", ce.id);
                        }
                    } break;
                    case EVENT_TYPE_HEARTBEAT_PREQUIT: {
                        auto ce = event_cast<f_event_heartbeat>(e);
                        if (timeout_item_exists(ce.id)) {
                            if (!timeout_items[ce.id].quit) {
                                // set item to timeout in e.heartbeat.time ms if it isnt unregistered until then
                                timeout_items[ce.id].heartbeat_answered = false;
                                timeout_items[ce.id].last_heartbeat_age = -real_sleep_time;
                                timeout_items[ce.id].timeout_ms = ce.time;
                                timeout_items[ce.id].quit = true;
                            } else {
                                MetaGui::logf("#W timeout_crash: received prequit for prequit timeout item #%d\n", ce.id);
                            }
                        } else {
                            //HACK just ignore this, is not an issue because some queues might unregister before the prequit has been acknowledged
                            // MetaGui::logf("#E timeout_crash: received prequit for unknown timeout item #%d\n", e.heartbeat.id);
                        }
                    } break;
                    case EVENT_TYPE_HEARTBEAT_RESET: {
                        auto ce = event_cast<f_event_heartbeat>(e);
                        if (timeout_item_exists(ce.id)) {
                            if (!timeout_items[ce.id].quit) {
                                timeout_items[ce.id].last_heartbeat_age -= real_sleep_time;
                            } else {
                                MetaGui::logf("#W timeout_crash: received reset for prequit timeout item #%d\n", ce.id);
                            }
                        } else {
                            MetaGui::logf("#E timeout_crash: received reset for unknown timeout item #%d\n", ce.id);
                        }
                    } break;
                    default: {
                        MetaGui::logf("#W timeout_crash: received unexpected event, type: %d\n", e.base.type);
                    } break;
                }
                f_event_queue_pop(&inbox, &e, 0);
            }
            // add slept time to all heartbeat ages (except new ones), might've woken up earlier than expected
            map_iter = timeout_items.begin();
            while (map_iter != timeout_items.end()) {
                timeout_item& tii = map_iter->second;
                tii.last_heartbeat_age += real_sleep_time;
                if (tii.last_heartbeat_age >= tii.timeout_ms) {
                    // heartbeat deadline
                    if (tii.heartbeat_answered)  {
                        tii.last_heartbeat_age -= tii.timeout_ms;
                        tii.heartbeat_answered = false;
                        f_event_any es;
                        f_event_create_heartbeat(&es, EVENT_TYPE_HEARTBEAT, tii.id, 0);
                        f_event_queue_push(tii.q, &es);
                    } else {
                        // if the item has not responded to our heartbeat in timeout ms, quit
                        if (!tii.quit) {
                            fprintf(stderr, "[FATAL] timeout item #%d failed to provide heartbeat: %s\n", tii.id, tii.name);
                        } else {
                            fprintf(stderr, "[FATAL] timeout item #%d failed to unregister in time: %s\n", tii.id, tii.name);
                        }
                        exit(1);
                    }
                }
                map_iter++;
            }
            m.unlock();

        }
    }
    

    void TimeoutCrash::start()
    {
        runner = std::thread(&TimeoutCrash::loop, this);
    }
    
    void TimeoutCrash::join()
    {
        runner.join();
    }
    
    TimeoutCrash::timeout_info TimeoutCrash::register_timeout_item(f_event_queue* target_queue, const char* name, int initial_delay, int timeout_ms)
    {
        m.lock();
        uint32_t item_id = next_id++;
        if (timeout_item_exists(name)) {
            MetaGui::logf("#W registering timeout item #%d with an already existing name: %s\n", item_id, name);
        }
        timeout_items[item_id] = timeout_item(target_queue, item_id, name, initial_delay, timeout_ms);
        f_event_any es;
        f_event_create_heartbeat(&es, EVENT_TYPE_HEARTBEAT, item_id, 0);
        f_event_queue_push(timeout_items[item_id].q, &es);
        m.unlock();
        // force wakeup the looping thread to re-calculate nearest deadline, also dont add sleep to the new one
        f_event_create_heartbeat(&es, EVENT_TYPE_HEARTBEAT_RESET, item_id, 0);
        f_event_queue_push(&inbox, &es);
        return timeout_info{item_id, &inbox};
    }

    void TimeoutCrash::unregister_timeout_item(uint32_t id)
    {
        m.lock();
        if (!timeout_item_exists(id)) {
            MetaGui::logf("#W attempted un-register of unknown timeout item #%d\n", next_id);
            m.unlock();
            return;
        }
        // free name string and erase from map
        free(timeout_items[id].name);
        timeout_items.erase(id);
        m.unlock();
    }

    bool TimeoutCrash::timeout_item_exists(uint32_t id)
    {
        return timeout_items.find(id) != timeout_items.end();
    }

    bool TimeoutCrash::timeout_item_exists(const char* name)
    {
        std::unordered_map<uint32_t, timeout_item>::iterator map_iter = timeout_items.begin();
        while (map_iter != timeout_items.end()) {
            if (strcmp(map_iter->second.name, name) == 0) {
                return true;
            }
            map_iter++;
        }
        return false;
    }

}
