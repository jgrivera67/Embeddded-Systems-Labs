/**
 * @file runtime_checks.h
 *
 * Compile-time checking macros
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_COMPILE_TIME_CHECKS_H_
#define SOURCES_BUILDING_BLOCKS_COMPILE_TIME_CHECKS_H_

#if __STDC_VERSION__ == 201112L
/**
 * Macro to check compile-time assertions
 */
#define C_ASSERT(_cond) \
    _Static_assert(_cond, #_cond)

#else
#define C_ASSERT(_cond) \
        extern const char c_assert_dummy_decl[(_cond) ? 1 : -1]

#endif /* __STDC_VERSION__ == 201112L */

#endif /* SOURCES_BUILDING_BLOCKS_COMPILE_TIME_CHECKS_H_ */
