#include <atomic>
#include <cstdbool>
#include <cstddef>
#include <cstdint>

#include "mirabel/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cfg_reference_s cfg_reference;
struct cfg_reference_s {
    std::atomic<cfg_reference*> succ;
    std::atomic<uint32_t> ref_count;
    cfg_ovac ovac;
    bool changed;
};

struct config_registry_s {
    cfg_ovac* root;

};

#ifdef __cplusplus
}
#endif
