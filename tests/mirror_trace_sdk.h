/* Host transport for fixed metadata addresses and pinned patch ABI. */
#include "capture_sdk.h"
typedef void (*patch_hook_function_cbr)(uint32_t*,uint32_t*,uint32_t);
uint32_t read_value(uint8_t*addr,int instruction);
int patch_hook_function(uintptr_t addr,uint32_t original,patch_hook_function_cbr hook,const char*description);
uint32_t kaleido_trace_test_read32(uintptr_t address);
uint16_t kaleido_trace_test_read16(uintptr_t address);
void *kaleido_trace_test_sensor_pointer(unsigned address);
unsigned get_us_clock(void);
int get_current_task_id(void);
struct edmac_info {unsigned off1a,off1b,off2a,off2b,off3,xa,xb,ya,yb,xn,yn;};
struct edmac_info edmac_get_info(uint32_t channel);
uint32_t edmac_get_flags(uint32_t channel);
