#ifndef SRC_ONIG_SEARCH_TRACER_H_
#define SRC_ONIG_SEARCH_TRACER_H_

#if !defined(_WIN32)
#include <time.h>
#include <sys/time.h>
#endif  // !defined(_WIN32)

#include <vector>

class OnigResult;

class OnigSearchTracer {
  public:
    struct Trace {
      Trace();
      Trace(int index, int matchedAt, float duration);

      bool DidMatch() const { return 0 <= matchedAt; }

      int index;
      int matchedAt;
      float duration;
    };

    explicit OnigSearchTracer(float slownessThreshold);
    ~OnigSearchTracer();

    void WillSearch(int regExpIndex);
    void DidSearch(OnigResult* match);

    size_t SlowSearchCount() const { return slowSearches.size(); }
    const Trace& SlowSearchAt(size_t i) const { return slowSearches[i]; }

  private:
    float slownessThreshold;  // The slow-search threshould, in seconds.
    std::vector<Trace> slowSearches;
    int      regExpIndex;
#if !defined(_WIN32)
    timespec startedAt;
#endif
};

#endif  // SRC_ONIG_SEARCH_TRACER_H_
