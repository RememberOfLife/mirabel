#include "mirabel/alloc.h"
#define malloc mirabel_malloc
#define realloc mirabel_realloc
#define free mirabel_free
#define ROSALIA_VECTOR_IMPLEMENTATION
#include "rosalia/vector.h"
