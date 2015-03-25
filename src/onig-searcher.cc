#include "onig-searcher.h"
#include "onig-search-tracer.h"
#include "onig-string-context.h"
#include "unicode-utils.h"

shared_ptr<OnigResult> OnigSearcher::Search(shared_ptr<OnigStringContext> source, int charOffset, OnigSearchTracer* tracer) {
  int byteOffset = charOffset;
  if (source->has_multibyte_characters()) {
#ifdef _WIN32
    byteOffset = UnicodeUtils::bytes_in_characters(source->utf16_value(), charOffset);
#else
    byteOffset = UnicodeUtils::bytes_in_characters(source->utf8_value(), charOffset);
#endif
  }

  int bestLocation = 0;
  shared_ptr<OnigResult> bestResult;
  cache.Init(source, byteOffset);

  for (size_t i = 0; i < regExps.size(); ++i) {
    OnigRegExp *regExp = regExps[i].get();
    if (tracer)
      tracer->WillSearch(i);
    shared_ptr<OnigResult> result = cache.Search(regExp, source, byteOffset);
    if (tracer)
      tracer->DidSearch(result.get());
    if (result != NULL && result->Count() > 0) {
      int location = result->LocationAt(0);
      if (source->has_multibyte_characters()) {
        location =  UnicodeUtils::characters_in_bytes(source->utf8_value(), location);
      }

      if (bestResult == NULL || location < bestLocation) {
        bestLocation = location;
        bestResult = result;
      }

      if (location == charOffset) {
        break;
      }
    }
  }

  return bestResult;
}
