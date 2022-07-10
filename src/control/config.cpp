#include <atomic>
#include <cstdbool>
#include <cstddef>
#include <cstdint>

#include "mirabel/config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct cfg_reference_s {
    cfg_ovac ovac;
    std::atomic<cfg_reference*> pred; // older value, if this is NULL and the ref_count reaches 0, drop this ref from the cache and propagate along the succ chain by: decrease current ref by one, increasing ref of succ by one, then freeing this one, then return succ if no newer ones, if newer ones continue forwards
    std::atomic<cfg_reference*> succ; // newer value
    std::atomic<uint32_t> ref_count;
    bool changed;
};

typedef struct config_registry_s {
    cfg_ovac* root;
    //TODO the actual cfg_reference entry cache
    //TODO map from uint32_t -> cfg_reference*
    //TODO
} config_registry;

#ifdef __cplusplus
}
#endif
