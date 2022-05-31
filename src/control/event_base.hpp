#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace Control {

    //TODO managed strings, or use std::string
    //TODO blob manager struct

    struct f_event; // forward declare from event.hpp

    struct event_serializer {
        static const bool is_plain = false;
        //TODO plain size? for move
        virtual size_t size(f_event* e) = 0;
        virtual void serialize(f_event* e, void** buf) = 0;
        virtual int deserialize(f_event* e, void** buf, void* buf_end) = 0;
        virtual void copy(f_event* to, f_event* from) = 0;
        virtual void destroy(f_event* e) = 0;
    };

    template<class EVENT, class ...EVENT_SERIALIZERS>
    struct event_serializer_compositor : public event_serializer {

        static event_serializer* instance()
        {
            static event_serializer_compositor<EVENT, EVENT_SERIALIZERS...> the_instance = event_serializer_compositor<EVENT, EVENT_SERIALIZERS...>();
            return &the_instance;
        }

        template<class X, class FIRST, class ...REST>
        static constexpr char EVENT::* get_first_member_impl(EVENT* e, char EVENT::* min)
        {
            return ((char*)&(e->*FIRST::get_first_member(e)) < (char*)&(e->*min))
                ? get_first_member_impl<X, REST...>(e, (char EVENT::*)FIRST::get_first_member(e))
                : get_first_member_impl<X, REST...>(e, min);
        }
        template<class X>
        static constexpr char EVENT::* get_first_member_impl(EVENT* e, char EVENT::* min)
        {
            return min;
        }
        template<class X, class FIRST, class ...REST>
        static constexpr char EVENT::* get_first_member_prefix_impl(EVENT* e)
        {
            return get_first_member_impl<X, FIRST, REST...>(e, (char EVENT::*)FIRST::get_first_member(e));
        }
        static constexpr char EVENT::* get_first_member(EVENT* e)
        {
            return get_first_member_prefix_impl<void, EVENT_SERIALIZERS...>(e);
        }

        template<class X, class FIRST, class ...REST>
        static constexpr size_t size_impl(EVENT* e, size_t s)
        {
            return size_impl<X, REST...>(e, s + FIRST::static_size(e));
        }
        template<class X>
        static constexpr size_t size_impl(EVENT* e, size_t s)
        {
            return s;
        }
        template<class X, class FIRST, class ...REST>
        static constexpr size_t plain_size_impl(EVENT* e)
        {
            return ((char*)&(e->*get_first_member(e)) - (char*)e);
        }
        template<class X>
        static constexpr size_t plain_size_impl(EVENT* e)
        {
            return sizeof(EVENT);
        }
        static constexpr size_t plain_size(f_event* e)
        {
            return sizeof(size_t) + plain_size_impl<void, EVENT_SERIALIZERS...>((EVENT*)e);
        }
        size_t size(f_event* e) override
        {
            return plain_size((EVENT*)e) + size_impl<void, EVENT_SERIALIZERS...>((EVENT*)e, (size_t)0);
        }

        template<class X, class FIRST, class ...REST>
        static void serialize_impl(EVENT* e, void** buf)
        {
            FIRST::static_serialize(e, buf);
            serialize_impl<X, REST...>(e, buf);
        }
        template<class X>
        static void serialize_impl(EVENT* e, void** buf)
        {}
        void serialize(f_event* e, void** buf) override
        {
            size_t ps = plain_size(e) - sizeof(size_t);
            size_t rs = size(e);
            *(size_t*)*buf = rs; // the total event size written at the very front INCLUDES itself
            *buf = (char*)*buf + sizeof(size_t);
            memcpy(*buf, (void*)e, ps); //HACK relies on static event layout
            *buf = (char*)*buf + ps;
            serialize_impl<void, EVENT_SERIALIZERS...>((EVENT*)e, buf);
        }
        
        template<class X, class FIRST, class ...REST>
        static int deserialize_impl(EVENT* e, void** buf, void* buf_end)
        {
            if (FIRST::static_deserialize(e, buf, buf_end)) {
                return 1;
            }
            return deserialize_impl<X, REST...>(e, buf, buf_end);
        }
        template<class X>
        static int deserialize_impl(EVENT* e, void** buf, void* buf_end)
        {
            return 0;
        }
        int deserialize(f_event* e, void** buf, void* buf_end) override
        {
            assert(*(size_t*)*buf == (char*)buf_end - (char*)*buf);
            size_t ps = plain_size(e) - sizeof(size_t);
            if ((char*)buf_end - (char*)*buf < ps) {
                return 1;
            }
            *buf = (char*)*buf + sizeof(size_t);
            memcpy((void*)e, *buf, ps);
            *buf = (char*)*buf + ps;
            return deserialize_impl<void, EVENT_SERIALIZERS...>((EVENT*)e, buf, buf_end);
        }

        template<class X, class FIRST, class ...REST>
        static void copy_impl(EVENT* to, EVENT* from)
        {
            FIRST::static_copy(to, from);
            copy_impl<X, REST...>(to, from);
        }
        template<class X>
        static void copy_impl(EVENT* to, EVENT* from)
        {}
        void copy(f_event* to, f_event* from) override
        {
            memcpy(to, from, plain_size(from) - sizeof(size_t));
            copy_impl<void, EVENT_SERIALIZERS...>((EVENT*)to, (EVENT*)from);
        }

        template<class X, class FIRST, class ...REST>
        static void destroy_impl(EVENT* e)
        {
            FIRST::static_destroy(e);
            destroy_impl<X, REST...>(e);
        }
        template<class X>
        static void destroy_impl(EVENT* e)
        {}
        void destroy(f_event* e) override
        {
            destroy_impl<void, EVENT_SERIALIZERS...>((EVENT*)e);
        }

    };

    template<class EVENT>
    struct event_plain_serializer : public event_serializer_compositor<EVENT> {
        static const bool is_plain = true;
    };

    template<class EVENT, char* EVENT::*...STRINGS>
    struct event_string_serializer_impl : public event_serializer {

        static event_serializer* instance()
        {
            static event_string_serializer_impl<EVENT, STRINGS...> the_instance = event_string_serializer_impl<EVENT, STRINGS...>();
            return &the_instance;
        }

        template<class X, char* EVENT::*FIRST, char* EVENT::*...REST>
        static constexpr char* EVENT::* get_first_member_impl(EVENT* e, char* EVENT::* min)
        {
            return ((char*)&(e->*min) < (char*)&(e->*FIRST)) ? get_first_member_impl<X, REST...>(e, min) : get_first_member_impl<X, REST...>(e, FIRST);
        }
        template<class X>
        static constexpr char* EVENT::* get_first_member_impl(EVENT* e, char* EVENT::* min)
        {
            return min;
        }
        template<class X, char* EVENT::*FIRST, char* EVENT::*...REST>
        static constexpr char* EVENT::* get_first_member_prefix_impl(EVENT* e)
        {
            return get_first_member_impl<X, FIRST, REST...>(e, FIRST);
        }
        static constexpr char* EVENT::* get_first_member(EVENT* e)
        {
            return get_first_member_prefix_impl<void, STRINGS...>(e);
        }

        template<class X, char* EVENT::*FIRST, char* EVENT::*...REST>
        static constexpr size_t size_impl(EVENT* e, size_t s)
        {
            return size_impl<X, REST...>(e, s + (e->*FIRST && strlen(e->*FIRST) > 0 ? strlen(e->*FIRST) : 1) + 1);
        }
        template<class X>
        static constexpr size_t size_impl(EVENT* e, size_t s)
        {
            return s;
        }
        static constexpr size_t static_size(f_event* e)
        {
            return size_impl<void, STRINGS...>((EVENT*)e, 0);
        }
        size_t size(f_event* e) override
        {
            return static_size(e);
        }

        template<class X, char* EVENT::*FIRST, char* EVENT::*...REST>
        static void serialize_impl(EVENT* e, void** buf)
        {
            size_t str_len = 0;
            if (e->*FIRST) {
                str_len = strlen(e->*FIRST) + 1;
            }
            // str_len is 1 if NULL or empty"", otherwise strlen+1
            if (str_len <= 1) {
                *((char*)*buf) = '\0';
                if (str_len == 0) {
                    *((uint8_t*)*buf+1) = 0x00; // NULL becomes 0x0000
                } else {
                    *((uint8_t*)*buf+1) = 0xFF; // empty"" becomes 0x00FF
                }
                str_len = 2;
            } else {
                memcpy((char*)*buf, e->*FIRST, str_len);
            }
            *buf = (char*)*buf + str_len;
            serialize_impl<EVENT, REST...>(e, buf);
        }
        template<class X>
        static void serialize_impl(EVENT* e, void** buf)
        {}
        static void static_serialize(f_event* e, void** buf)
        {
            serialize_impl<void, STRINGS...>((EVENT*)e, buf);
        }
        void serialize(f_event* e, void** buf) override
        {
            static_serialize(e, buf);
        }
        
        template<class X, char* EVENT::*FIRST, char* EVENT::*...REST>
        static int deserialize_impl(EVENT* e, void** buf, void* buf_end)
        {
            size_t max_str_size = (char*)buf_end - (char*)*buf;
            const void* found = memchr(*buf, '\0', max_str_size); 
            if (!found || max_str_size < 2) {
                return 1;
            }
            size_t str_size = (char*)found - (char*)*buf + 1;
            if (str_size == 1 && *((uint8_t*)*buf + 1) == 0x00) {
                e->*FIRST = NULL;
                str_size = 2;
            } else {
                e->*FIRST = (char*)malloc(str_size);
                memcpy(e->*FIRST, *buf, str_size);
                if (str_size == 1) {
                    str_size += 1;
                }
            }
            *buf = (char*)*buf + str_size;
            return deserialize_impl<X, REST...>(e, buf, buf_end);
        }
        template<class X>
        static int deserialize_impl(EVENT* e, void** buf, void* buf_end)
        {
            return 0;
        }
        static int static_deserialize(f_event* e, void** buf, void* buf_end)
        {
            return deserialize_impl<void, STRINGS...>((EVENT*)e, buf, buf_end);
        }
        int deserialize(f_event* e, void** buf, void* buf_end) override
        {
            return static_deserialize(e, buf, buf_end);
        }

        template<class X, char* EVENT::*FIRST, char* EVENT::*...REST>
        static void copy_impl(EVENT* to, EVENT* from)
        {
            //BUG ? we do not free whatever already was inside the target event before pasting our copy over it
            to->*FIRST = from->*FIRST ? strdup(from->*FIRST) : NULL;
            copy_impl<X, REST...>(to, from);
        }
        template<class X>
        static void copy_impl(EVENT* to, EVENT* from)
        {}
        static void static_copy(f_event* to, f_event* from)
        {
            copy_impl<void, STRINGS...>((EVENT*)to, (EVENT*)from);
        }
        void copy(f_event* to, f_event* from) override
        {
            static_copy(to, from);
        }

        template<class X, char* EVENT::*FIRST, char* EVENT::*...REST>
        static void destroy_impl(EVENT* e)
        {
            free(e->*FIRST);
            e->*FIRST = NULL;
            destroy_impl<X, REST...>(e);
        }
        template<class X>
        static void destroy_impl(EVENT* e)
        {}
        static void static_destroy(f_event* e)
        {
            destroy_impl<void, STRINGS...>((EVENT*)e);
        }
        void destroy(f_event* e) override
        {
            static_destroy(e);
        }

    };
    
    template<class EVENT, char* EVENT::*...STRINGS>
    struct event_string_serializer : public event_serializer_compositor<EVENT, event_string_serializer_impl<EVENT, STRINGS...>> {};

    //TODO blob serializer

}
