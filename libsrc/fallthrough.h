/*
 * OpenCReports fallthrough header
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#ifndef _FALLTHROUGH_H_
#define _FALLTHROUGH_H_

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if defined(__has_cpp_attribute) && defined(__clang__)
/* We do not do the same trick as __has_attribute because parsing
 * clang::fallthrough in the preprocessor fails in GCC. */
#define HAS_CLANG_FALLTHROUGH  __has_cpp_attribute(clang::fallthrough)
#else
#define HAS_CLANG_FALLTHROUGH 0
#endif

#if __cplusplus >= 201703L || __STDC_VERSION__ > 201710L
/* Standard C++17/C23 attribute */
#define FALLTHROUGH [[fallthrough]]
#elif HAS_CLANG_FALLTHROUGH
/* Clang++ specific */
#define FALLTHROUGH [[clang::fallthrough]]
#elif __has_attribute(fallthrough)
/* Non-standard but supported by at least gcc and clang */
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH do { } while(0)
#endif

#endif
