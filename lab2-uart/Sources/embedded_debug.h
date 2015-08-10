/*
 * embedded_debug.h - embedded debugging facilities
 *
 * @author: German Rivera
 */
#ifndef SOURCES_EMBEDDED_DEBUG_H_
#define SOURCES_EMBEDDED_DEBUG_H_

/**
 * Macro to give a hint to the compiler for static branch prediction,
 * for infrequently executed code paths, such as error cases.
 */
#define _INFREQUENTLY_TRUE_(_condition)	__builtin_expect((_condition), 0)

/**
 * Macro to check run-time assertions in embedded code. If an assertion is
 * false, execution is trapped in a tight infinite loop.
 */
#define ASSERT(_cond) \
		do { 																\
			if (_INFREQUENTLY_TRUE_(!(_cond))) { 							\
				capture_assert_failure(#_cond, __func__, __FILE__, 			\
									   __LINE__);							\
			}																\
		} while (0)

void capture_assert_failure(const char *cond_str, const char *func_name,
						    const char *file_name, int line);

#endif /* SOURCES_EMBEDDED_DEBUG_H_ */
