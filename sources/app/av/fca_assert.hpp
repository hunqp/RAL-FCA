#ifndef __FCA_ASSERT_HPP__
#define __FCA_ASSERT_HPP__

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "sys_log.h"

#define FCA_API_ASSERT(expr)                        \
    do                                              \
    {                                               \
        if (!(expr))                                \
        {                                           \
            RAM_SYSE("ASSERT FAILED: %s\n", #expr); \
            assert(expr);                           \
        }                                           \
    } while (0)

#endif /* __FCA_ASSERT_HPP__ */