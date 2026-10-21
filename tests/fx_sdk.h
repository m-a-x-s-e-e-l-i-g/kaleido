/* Test transport only; real shared camera adapter is compiled unchanged. */
#include "capture_sdk.h"
int module_get_next_loaded(int index);
const char *module_get_name(int index);
int module_display_filter_enabled(void);
void fx_test_committed_canvas(void);
void *memcpy64(void*dest,void*src,size_t size);
