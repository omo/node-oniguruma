#ifndef SRC_ONIG_SCANNER_H_
#define SRC_ONIG_SCANNER_H_

#include <string>
#include <vector>
#include <memory>

#include "nan.h"
#include "onig-cache.h"
#include "onig-searcher.h"

using ::v8::Array;
using ::v8::Function;
using ::v8::Handle;
using ::v8::Number;
using ::v8::Object;
using ::v8::Persistent;
using ::v8::String;
using ::v8::Value;

using ::std::string;
using ::std::shared_ptr;
using ::std::unique_ptr;
using ::std::vector;

class OnigRegExp;
class OnigResult;
class OnigStringContext;
class OnigSearchTracer;

class OnigScanner : public node::ObjectWrap {
 public:
  static void Init(Handle<Object> target);

 private:
  static NAN_METHOD(New);
  static NAN_METHOD(FindNextMatch);
  static NAN_METHOD(FindNextMatchSync);
  static NAN_METHOD(EnableTracing);
  explicit OnigScanner(Handle<Array> sources);
  ~OnigScanner();

  void FindNextMatch(Handle<String> v8String, Handle<Number> v8StartLocation, Handle<Function> v8Callback);
  Handle<Value> FindNextMatchSync(Handle<String> v8String, Handle<Number> v8StartLocation);
  void EnableTracing(Handle<Number> v8SlownessThreshold);
  Handle<Value> CaptureIndicesForMatch(OnigResult* result, shared_ptr<OnigStringContext> source);
  Handle<Value> SlowSearches(OnigSearchTracer* tracer) const;

  Persistent<Array> sources;
  vector<shared_ptr<OnigRegExp>> regExps;
  shared_ptr<OnigSearcher> searcher;
  shared_ptr<OnigCache> asyncCache;
  shared_ptr<OnigStringContext> lastSource;
  unique_ptr<OnigSearchTracer> searchTracer;
};

#endif  // SRC_ONIG_SCANNER_H_
