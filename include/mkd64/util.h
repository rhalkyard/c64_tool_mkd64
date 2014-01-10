#ifndef MKD64_UTIL_H
#define MKD64_UTIL_H

/** @file util.h
 * collection of generic utility functions for mkd64 and modules
 * This file contains utility functions that do not directly belong to one
 * of mkd64's classes.
 */

#include <mkd64/common.h>

/** Get random integer in a given range
 * The random number generator is initialized on the first call
 * @param min minimum number to return
 * @param max maximum number to return
 * @return random number
 */
DECLEXPORT int randomNum(int min, int max);

/** Try parsing an integer from a given string
 * For tryParseInt, the string must contain a number represented only by
 * decimal digits and an optional minus ('-') at the beginning. Otherwise
 * it will return false and the content of the result is undefined.
 * @param str the string to parse
 * @param result pointer an integer for placing the result
 * @return 1 (true) if parsed correctly, 0 (false) otherwise
 */
DECLEXPORT int tryParseInt(const char *str, int *result);

/** Check for presence of a argument and emit warning message
 * This is a convenience function that checks whether an option has an
 * argument and directly prints a warning message, if this is not the
 * expected case. For missing arguments, the message will warn that the
 * option will be ignored, for extra arguments, the message will read that
 * the argument will be ignored. This is the recommended behavior for mkd64
 * modules.
 * @param opt the option
 * @param arg the actual argument (or, 0)
 * @param isFileOpt 1 if this is a file option, 0 if it is a global option
 * @param argExpected 1 if the option needs an argument, 0 if not
 * @param modid the id string of the module, or 0 if the caller is mkd64
 * @return 1 if expectation is met, 0 if not (and warning was printed)
 */
DECLEXPORT int checkArgAndWarn(char opt, const char *arg, int isFileOpt,
        int argExpected, const char *modid);

#endif
/* vim: et:si:ts=4:sts=4:sw=4
*/
