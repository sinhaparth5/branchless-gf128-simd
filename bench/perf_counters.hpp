#pragma once

#include <cstdint>

#if defined(__linux__)
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstring>
#endif

namespace bench {

// User-space hardware counters around one region, read through
// perf_event_open. Linux only. Elsewhere, or when the kernel refuses (for
// example perf_event_paranoid > 2 or a VM without a PMU), ok() is false.
class Counters {
 public:
  enum { kCycles, kInstructions, kBranches, kBranchMisses, kCount };

  Counters() {
#if defined(__linux__)
    const std::uint64_t cfg[kCount] = {PERF_COUNT_HW_CPU_CYCLES, PERF_COUNT_HW_INSTRUCTIONS,
                                       PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
                                       PERF_COUNT_HW_BRANCH_MISSES};
    ok_ = true;
    for (int i = 0; i < kCount; ++i) {
      perf_event_attr pe;
      std::memset(&pe, 0, sizeof pe);
      pe.type = PERF_TYPE_HARDWARE;
      pe.size = sizeof pe;
      pe.config = cfg[i];
      pe.disabled = 1;
      pe.exclude_kernel = 1;
      pe.exclude_hv = 1;
      fd_[i] = static_cast<int>(syscall(SYS_perf_event_open, &pe, 0, -1, -1, 0));
      if (fd_[i] < 0) ok_ = false;
    }
#endif
  }

  ~Counters() {
#if defined(__linux__)
    for (int fd : fd_)
      if (fd >= 0) close(fd);
#endif
  }

  Counters(const Counters&) = delete;
  Counters& operator=(const Counters&) = delete;

  bool ok() const { return ok_; }

  void start() {
#if defined(__linux__)
    if (!ok_) return;
    for (int fd : fd_) ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    for (int fd : fd_) ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
#endif
  }

  void stop() {
#if defined(__linux__)
    if (!ok_) return;
    for (int fd : fd_) ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    for (int i = 0; i < kCount; ++i)
      if (read(fd_[i], &v_[i], sizeof v_[i]) != sizeof v_[i]) ok_ = false;
#endif
  }

  std::uint64_t value(int i) const { return v_[i]; }

 private:
  bool ok_ = false;
#if defined(__linux__)
  int fd_[kCount] = {-1, -1, -1, -1};
#endif
  std::uint64_t v_[kCount] = {};
};

}  // namespace bench
