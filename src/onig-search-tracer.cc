#include "onig-search-tracer.h"
#include <cstdint>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "oniguruma.h"
#include "onig-result.h"


namespace {

// FIXME: This doesn't work on Windows so far.
#if !defined(_WIN32)
// See:
// - https://gist.github.com/jbenet/1087739
// - http://stackoverflow.com/questions/5167269/
void currentUtcTime(struct timespec *ts) {
#ifdef __MACH__
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, ts);
#endif
}

float deltaSeconds(const timespec& from, const timespec& to) {
  int64_t sec = to.tv_sec - from.tv_sec;
  int64_t nsec = to.tv_nsec - from.tv_nsec;
  return static_cast<float>(sec) + static_cast<float>(nsec)/1000000000.0f;
}
#endif  // !defined(_WIN32)

}  // namespace

OnigSearchTracer::Trace::Trace()
    : index(0), duration(0.0f) {
}

OnigSearchTracer::Trace::Trace(int index, int matchedAt, float duration)
    : index(index), matchedAt(matchedAt), duration(duration) {
}

OnigSearchTracer::OnigSearchTracer(float slownessThreshold)
    : slownessThreshold(slownessThreshold),
      regExpIndex(0) {
}

OnigSearchTracer::~OnigSearchTracer() {
}

void OnigSearchTracer::WillSearch(int regExpIndex) {
  this->regExpIndex = regExpIndex;
#if !defined(_WIN32)
  currentUtcTime(&this->startedAt);
#endif
}

void OnigSearchTracer::DidSearch(OnigResult* match) {
#if !defined(_WIN32)
  timespec endedAt;
  currentUtcTime(&endedAt);
  float delta = deltaSeconds(startedAt, endedAt);
  if (delta < slownessThreshold)
    return;
  if (match) {
    slowSearches.push_back(Trace(regExpIndex, match->LocationAt(0), delta));
  } else {
    slowSearches.push_back(Trace(regExpIndex, -1, delta));
  }
#endif
}
