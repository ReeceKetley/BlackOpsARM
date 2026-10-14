#pragma once
#include <cstdarg>
namespace bo1::android {
using AssertionReporter = void (*)(const char *);
// Engine callbacks have process-lifetime code storage. Clearing/replacing the
// reporter is atomic; it does not retire code already acquired by an assertion.
void SetAssertionReporter(AssertionReporter reporter);
[[noreturn]] void ReportAssertion(const char *filename,int line,int type,
                                 const char *format,va_list arguments);
}
