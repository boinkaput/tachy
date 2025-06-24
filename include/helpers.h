#pragma once

#define TACHY_PASTE_(_0, _1) _0 ## _1
#define TACHY_PASTE(_0, _1) TACHY_PASTE_(_0, _1)
#define TACHY_STRINGIFY_(_0) #_0
#define TACHY_STRINGIFY(_0) TACHY_STRINGIFY_(_0)

#define TACHY_LABEL __LINE__

#define TACHY_PASTE5(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define TACHY_TRIGGER_PARENTHESIS(...) ,
#define TACHY_IS_EMPTY_CASE_0001 ,
#define TACHY_ARG3(_0, _1, _2, ...) _2
#define TACHY_HAS_COMMA(...) TACHY_ARG3(__VA_ARGS__, 1, 0)
#define TACHY_CHECK_ISEMPTY(_0, _1, _2, _3)                                     \
    TACHY_HAS_COMMA(TACHY_PASTE5(TACHY_IS_EMPTY_CASE_, _0, _1, _2, _3))

#define TACHY_ISEMPTY(...)                                                      \
    TACHY_CHECK_ISEMPTY(TACHY_HAS_COMMA(__VA_ARGS__),                           \
                       TACHY_HAS_COMMA(TACHY_TRIGGER_PARENTHESIS __VA_ARGS__),  \
                       TACHY_HAS_COMMA(__VA_ARGS__ ()),                         \
                       TACHY_HAS_COMMA(TACHY_TRIGGER_PARENTHESIS __VA_ARGS__ ()))

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L) // C23 or later
    #define TACHY_FALLTHROUGH [[fallthrough]]
    #define TACHY_UNUSED [[maybe_unused]]
#elif defined(__GNUC__) || defined(__clang__)
    #define TACHY_FALLTHROUGH __attribute__((fallthrough))
    #define TACHY_UNUSED __attribute__((unused))
#else
    #define TACHY_FALLTHROUGH
    #define TACHY_UNUSED
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) // C11 or later
    #define TACHY_STATIC_ASSERT(cond, msg) _Static_assert(cond, TACHY_STRINGIFY(msg))
#else
    #define TACHY_STATIC_ASSERT(cond, msg)                                      \
        typedef char TACHY_PASTE(static_assertion_at_line_, msg)[(cond) ? 1 : -1]
#endif
