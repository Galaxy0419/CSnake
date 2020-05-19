/* Copyright (c) 2019, William TANG <galaxyking0419@gmail.com> */
#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

/* always_inline keyword */
#ifdef _MSC_VER
#define restrict __restrict
#define always_inline static __forceinline
#endif

#ifdef __GNUC__
#define always_inline static __attribute__((always_inline))
#endif

#endif