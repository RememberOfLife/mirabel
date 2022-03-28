#pragma once

#include <mutex>
#include <thread>
#include <unordered_map>

#include "control/event_queue.hpp"

namespace Control {

    class TimeoutCrash {

        //TODO some way for a registered item to announce it will deconstruct, then timeoutcrash can watch over the deconstruction
        // crash if not unregistered before next heartbeat

        public:

            struct timeout_info { //TODO better name, heartbeat handle?
                uint32_t id;
                event_queue* q;
                void send_heartbeat();
            };

            std::thread runner;
            event_queue inbox;

            struct timeout_item {
                int32_t id;
                char* name;
                int timeout_ms;
                int last_heartbeat_age;
                bool heartbeat_answered;
                event_queue* q;
                timeout_item();
                timeout_item(event_queue* target_queue, uint32_t new_id, const char* new_name, int new_initial_delay, int new_timeout_ms);
            };

            uint32_t next_id = 1;
            std::unordered_map<uint32_t, timeout_item> timeout_items;
            std::mutex m;

            TimeoutCrash();
            ~TimeoutCrash();
            void loop();

            void start();
            void join();

            timeout_info register_timeout_item(event_queue* target_queue, const char* name, int initial_delay, int timeout_ms); // copies name into internals
            void unregister_timeout_item(uint32_t id);

            bool timeout_item_exists(uint32_t id);
            bool timeout_item_exists(const char* name);

    };

}
