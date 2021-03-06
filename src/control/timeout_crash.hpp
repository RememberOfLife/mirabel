#pragma once

#include <mutex>
#include <thread>
#include <unordered_map>

#include "mirabel/event_queue.h"

namespace Control {

    class TimeoutCrash {

        //TODO some way for a registered item to announce it will deconstruct, then timeoutcrash can watch over the deconstruction
        // crash if not unregistered before next heartbeat

        public:

            struct timeout_info { //TODO better name, heartbeat handle?
                uint32_t id;
                f_event_queue* q;
                void send_heartbeat();
                void pre_quit(uint32_t timeout);
            };

            std::thread runner;
            f_event_queue inbox;

            struct timeout_item {
                int32_t id;
                char* name;
                int timeout_ms;
                int last_heartbeat_age;
                bool heartbeat_answered;
                bool quit;
                f_event_queue* q;
                timeout_item();
                timeout_item(f_event_queue* target_queue, uint32_t new_id, const char* new_name, int new_initial_delay, int new_timeout_ms);
            };

            uint32_t next_id = 1;
            std::unordered_map<uint32_t, timeout_item> timeout_items;
            std::mutex m;

            TimeoutCrash();
            ~TimeoutCrash();
            void loop();

            void start();
            void join();

            timeout_info register_timeout_item(f_event_queue* target_queue, const char* name, int initial_delay, int timeout_ms); // copies name into internals
            void unregister_timeout_item(uint32_t id); //TODO maybe move this into timeout_info.unregister() using EVENT_TYPE_HEARTBEAT_UNREGISTER

            bool timeout_item_exists(uint32_t id);
            bool timeout_item_exists(const char* name);

    };

}
