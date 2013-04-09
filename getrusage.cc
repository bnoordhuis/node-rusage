/* Copyright (c) 2013, Ben Noordhuis <info@bnoordhuis.nl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE  // Want RUSAGE_THREAD on Linux.
#endif

#include "v8.h"
#include "node.h"
#include <errno.h>
#include <string.h>
#include <sys/resource.h>

namespace
{

#if (0 + NODE_MODULE_VERSION) >= 13
# define HANDLE_SCOPE(name)                                                   \
  v8::HandleScope name(v8::Isolate::GetCurrent())
#else
# define HANDLE_SCOPE(name)                                                   \
  v8::HandleScope name
#endif

v8::Handle<v8::Value> GetRUsage(const v8::Arguments& args);

void Initialize(v8::Handle<v8::Object> module)
{
  HANDLE_SCOPE(handle_scope);
  module->Set(v8::String::New("RUSAGE_SELF"),
              v8::Integer::New(RUSAGE_SELF));
  module->Set(v8::String::New("RUSAGE_CHILDREN"),
              v8::Integer::New(RUSAGE_CHILDREN));
#if defined(RUSAGE_LWP)  // Solaris, alias for RUSAGE_THREAD on Linux.
  module->Set(v8::String::New("RUSAGE_LWP"),
              v8::Integer::New(RUSAGE_LWP));
#endif
#if defined(RUSAGE_THREAD)  // Linux, FreeBSD (but not NetBSD or OpenBSD.)
  module->Set(v8::String::New("RUSAGE_THREAD"),
              v8::Integer::New(RUSAGE_THREAD));
#endif
  module->Set(v8::String::New("getrusage"),
              v8::FunctionTemplate::New(GetRUsage)->GetFunction());
}

v8::Handle<v8::Value> GetRUsage(const v8::Arguments& args)
{
  HANDLE_SCOPE(handle_scope);
  struct rusage ru;
  int who = (args.Length() > 0 ? args[0]->IntegerValue() : RUSAGE_SELF);
  int rc = getrusage(who, &ru);
  if (rc == -1) {
    char errmsg[256];
    snprintf(errmsg, sizeof(errmsg), "%s", strerror(errno));
    return v8::ThrowException(v8::Exception::Error(v8::String::New(errmsg)));
  }
  v8::Local<v8::Object> obj = v8::Object::New();
#define V(name)                                                               \
  do {                                                                        \
    v8::Local<v8::Object> t = v8::Object::New();                              \
    t->Set(v8::String::New("sec"), v8::Number::New(ru.name.tv_sec));          \
    t->Set(v8::String::New("usec"), v8::Number::New(ru.name.tv_usec));        \
    obj->Set(v8::String::New(#name), t);                                      \
  }                                                                           \
  while (0)
  V(ru_utime);
  V(ru_stime);
#undef V
  // POSIX only guarantees the existence of ru_utime and ru_stime but all
  // major Unices (Linux, Darwin, Solaris, FreebSD, NetBSD, OpenBSD) support
  // the subset below.
#define V(name) obj->Set(v8::String::New(#name), v8::Number::New(ru.name))
  V(ru_maxrss);
  V(ru_ixrss);
  V(ru_idrss);
  V(ru_isrss);
  V(ru_minflt);
  V(ru_majflt);
  V(ru_nswap);
  V(ru_inblock);
  V(ru_oublock);
  V(ru_msgsnd);
  V(ru_msgrcv);
  V(ru_nsignals);
  V(ru_nvcsw);
  V(ru_nivcsw);
#undef V
  return handle_scope.Close(obj);
}

}  // anonymous namespace

NODE_MODULE(getrusage, Initialize)
