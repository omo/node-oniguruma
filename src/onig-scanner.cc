#include "onig-scanner.h"
#include "onig-search-tracer.h"
#include "onig-string-context.h"
#include "onig-reg-exp.h"
#include "onig-result.h"
#include "onig-scanner-worker.h"
#include "unicode-utils.h"

using ::v8::Function;
using ::v8::FunctionTemplate;
using ::v8::HandleScope;
using ::v8::Local;
using ::v8::Null;

void OnigScanner::Init(Handle<Object> target) {
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(OnigScanner::New);
  tpl->SetClassName(NanNew<String>("OnigScanner"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->PrototypeTemplate()->Set(NanNew<String>("_findNextMatch"), NanNew<FunctionTemplate>(OnigScanner::FindNextMatch)->GetFunction());
  tpl->PrototypeTemplate()->Set(NanNew<String>("_findNextMatchSync"), NanNew<FunctionTemplate>(OnigScanner::FindNextMatchSync)->GetFunction());
  tpl->PrototypeTemplate()->Set(NanNew<String>("_enableTracing"), NanNew<FunctionTemplate>(OnigScanner::EnableTracing)->GetFunction());

  target->Set(NanNew<String>("OnigScanner"), tpl->GetFunction());
}

NODE_MODULE(onig_scanner, OnigScanner::Init)

NAN_METHOD(OnigScanner::New) {
  NanScope();
  OnigScanner* scanner = new OnigScanner(Local<Array>::Cast(args[0]));
  scanner->Wrap(args.This());
  NanReturnUndefined();
}

NAN_METHOD(OnigScanner::FindNextMatchSync) {
  NanScope();
  OnigScanner* scanner = node::ObjectWrap::Unwrap<OnigScanner>(args.This());
  NanReturnValue(scanner->FindNextMatchSync(Local<String>::Cast(args[0]), Local<Number>::Cast(args[1])));
}

NAN_METHOD(OnigScanner::FindNextMatch) {
  NanScope();
  OnigScanner* scanner = node::ObjectWrap::Unwrap<OnigScanner>(args.This());
  scanner->FindNextMatch(Local<String>::Cast(args[0]), Local<Number>::Cast(args[1]), Local<Function>::Cast(args[2]));
  NanReturnUndefined();
}

NAN_METHOD(OnigScanner::EnableTracing) {
  NanScope();
  OnigScanner* scanner = node::ObjectWrap::Unwrap<OnigScanner>(args.This());
  scanner->EnableTracing(Local<Number>::Cast(args[0]));
  NanReturnUndefined();
}

OnigScanner::OnigScanner(Handle<Array> sources)
    // FIXME: All these ifdefs are mess. We should have some compatibility layer
    // on NAN module to hide these API incompatibilities.
#if (0 == NODE_MAJOR_VERSION && 11 <= NODE_MINOR_VERSION) || (1 <= NODE_MAJOR_VERSION)
  : sources(Isolate::GetCurrent(), sources) {
#else
  : sources(Persistent<Array>::New(sources)) {
#endif
  int length = sources->Length();
  regExps.resize(length);

  for (int i = 0; i < length; i++) {
    String::Utf8Value utf8Value(sources->Get(i));
    regExps[i] = shared_ptr<OnigRegExp>(new OnigRegExp(string(*utf8Value), i));
  }

  searcher = shared_ptr<OnigSearcher>(new OnigSearcher(regExps));
  asyncCache = shared_ptr<OnigCache>(new OnigCache(length));
}

OnigScanner::~OnigScanner() {}

void OnigScanner::FindNextMatch(Handle<String> v8String, Handle<Number> v8StartLocation, Handle<Function> v8Callback) {
  int charOffset = v8StartLocation->Value();
  NanCallback *callback = new NanCallback(v8Callback);
  shared_ptr<OnigStringContext> source = shared_ptr<OnigStringContext>(new OnigStringContext(v8String));


  OnigScannerWorker *worker = new OnigScannerWorker(callback, regExps, source, charOffset, asyncCache);
  NanAsyncQueueWorker(worker);
}

Handle<Value> OnigScanner::FindNextMatchSync(Handle<String> v8String, Handle<Number> v8StartLocation) {
  if (!lastSource || !lastSource->IsSame(v8String))
    lastSource = shared_ptr<OnigStringContext>(new OnigStringContext(v8String));
  int charOffset = v8StartLocation->Value();

  shared_ptr<OnigResult> bestResult = searcher->Search(lastSource, charOffset, searchTracer.get());
  if (bestResult != NULL) {
    Local<Object> result = NanNew<Object>();
    result->Set(NanNew<String>("index"), NanNew<Number>(bestResult->Index()));
    result->Set(NanNew<String>("captureIndices"), CaptureIndicesForMatch(bestResult.get(), lastSource));
    if (searchTracer)
      result->Set(NanNew<String>("slowSearches"), SlowSearches(searchTracer.get()));
    return result;
  } else {
    return NanNull();
  }
}

void OnigScanner::EnableTracing(Handle<Number> v8SlownessThreshold) {
  searchTracer.reset(new OnigSearchTracer(v8SlownessThreshold->Value()));
}

Handle<Value> OnigScanner::CaptureIndicesForMatch(OnigResult* result, shared_ptr<OnigStringContext> source) {
  int resultCount = result->Count();
  Local<Array> captures = NanNew<Array>(resultCount);

  for (int index = 0; index < resultCount; index++) {
    int captureLength = result->LengthAt(index);
    int captureStart = result->LocationAt(index);

    if (source->has_multibyte_characters()) {
      captureLength = UnicodeUtils::characters_in_bytes(source->utf8_value() + captureStart, captureLength);
      captureStart = UnicodeUtils::characters_in_bytes(source->utf8_value(), captureStart);
    }

    Local<Object> capture = NanNew<Object>();
    capture->Set(NanNew<String>("index"), NanNew<Number>(index));
    capture->Set(NanNew<String>("start"), NanNew<Number>(captureStart));
    capture->Set(NanNew<String>("end"), NanNew<Number>(captureStart + captureLength));
    capture->Set(NanNew<String>("length"), NanNew<Number>(captureLength));
    captures->Set(index, capture);
  }

  return captures;
}

Handle<Value> OnigScanner::SlowSearches(OnigSearchTracer* tracer) const {
  size_t count = tracer->SlowSearchCount();
  Local<Array> slowSearches = NanNew<Array>(count);

#if (0 == NODE_MAJOR_VERSION && 11 <= NODE_MINOR_VERSION) || (1 <= NODE_MAJOR_VERSION)
  Local<Array> localSources = Local<Array>::New(Isolate::GetCurrent(), sources);
#else
  Local<Array> localSources = Local<Array>::New(sources);
#endif

  for (size_t i = 0; i < count; ++i) {
    Local<Object> traceValue = NanNew<Object>();
    const OnigSearchTracer::Trace& trace = tracer->SlowSearchAt(i);
    traceValue->Set(NanNew<String>("index"), NanNew<Number>(trace.index));
    traceValue->Set(NanNew<String>("pattern"), localSources->Get(trace.index));
    if (trace.DidMatch())
      traceValue->Set(NanNew<String>("matchedAt"), NanNew<Number>(trace.matchedAt));
    else
      traceValue->Set(NanNew<String>("matchedAt"), NanNull());
    traceValue->Set(NanNew<String>("duration"), NanNew<Number>(trace.duration));
    slowSearches->Set(i, traceValue);
  }

  return slowSearches;
}
