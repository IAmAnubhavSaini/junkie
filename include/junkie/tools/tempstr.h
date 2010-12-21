#ifndef TEMPSTR_100407
#define TEMPSTR_100407
#include <junkie/cpp.h>

/** @file
 * @brief Return temporary strings that does not need to be freed.
 *
 * This module returns temporary storage that does not need to be freed, of
 * maximum size TEMPSTR_SIZE, which is suitable to implement convertion to
 * string functions if the result is to be unused shortly after the call to
 * tempstr (typically, you use tempstr in thing_2_str function called
 * inside a printf call).
 */

#define TEMPSTR_SIZE 5000	// enough for any URL

char *tempstr(void);
char *tempstr_printf(char const *fmt, ...) a_la_printf_(1, 2);

char const *strnstr(char const *haystack, char const *needle, size_t len);

#endif
