#pragma once

#include <tools/vsx_rdtsc.h>
#include <unistd.h>

class vsx_perf
{
	uint64_t rdtsc_prev = 0;
public:

  // cache misses
  void cache_misses_start(bool include_kernel = false)
  {
  }

  void cache_misses_begin()
  {
  }

  long long cache_misses_end()
  {
    return 0;
  }

  void cache_misses_stop()
  {
  }

  // cpu instructions
  void cpu_instructions_start()
  {
  }

  inline void cpu_instructions_begin()
  {
    rdtsc_prev = vsx_rdtsc();
  }

  inline long long cpu_instructions_end()
  {
    return vsx_rdtsc() - rdtsc_prev;
  }

  void cpu_instructions_stop()
  {
  }

  /**
   * @brief memory_currently_used
   * Returns number of megabytes
   * @return
   */
  int memory_currently_used()
  {
    return memory_currently_used_bytes() / (1024 * 1024);
  }

  int memory_currently_used_bytes()
  {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (int) (pages * page_size) * 500;
  }

};
