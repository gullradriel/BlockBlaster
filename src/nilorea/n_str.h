/* Copyright (C) 2026 Nilorea Studio

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <https://www.gnu.org/licenses/>. */

/**\file n_str.h
 *  N_STR and string function declaration
 *\author Castagnier Mickael
 *\version 2.0
 *\date 05/02/14
 */

#ifndef N_STRFUNC
#define N_STRFUNC

/*! list of evil characters */
#define BAD_METACHARS "/-+&;`'\\\"|*?~<>^()[]{}$\n\r\t "

#ifdef __cplusplus
extern "C" {
#endif

/**\defgroup N_STR STRINGS: replacement to char *strings with dynamic resizing
  and boundary checking
  \addtogroup N_STR
  @{
  */

#include "n_common.h"
#include "n_list.h"

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/*! local strdup */
#define local_strdup(__src_)                                                   \
    ({                                                                         \
        char *str = NULL;                                                      \
        int len = strlen((__src_));                                            \
        Malloc(str, char, len + 5);                                            \
        if (!str) {                                                            \
            n_log(LOG_ERR, "Couldn't allocate %d byte for duplicating \"%s\"", \
                  (__src_));                                                   \
        } else {                                                               \
            for (int it = 0; it <= len; it++) {                                \
                str[it] = (__src_)[it];                                        \
            }                                                                  \
        }                                                                      \
        str;                                                                   \
    })

/*! Abort code to sped up pattern matching. Special thanks to Lars Mathiesen
 * <thorinn@diku.dk> for the ABORT code.*/
#define WILDMAT_ABORT -2
/*! What character marks an inverted character class? */
#define WILDMAT_NEGATE_CLASS '^'
/*! Do tar(1) matching rules, which ignore a trailing slash? */
#undef WILDMAT_MATCH_TAR_PATTERN

/*! Macro to quickly realloc a n_str */
#define resize_nstr(__nstr, new_size)                                          \
    ({                                                                         \
        if (__nstr && __nstr->data) {                                          \
            Reallocz(__nstr->data, char, __nstr->length, new_size);            \
            __nstr->length = new_size;                                         \
        }                                                                      \
    })

/*! Macro to quickly allocate and sprintf to a char * */
#define strprintf(__n_var, ...)                                                \
    ({                                                                         \
        if (!__n_var) {                                                        \
            NSTRBYTE needed = 0;                                               \
            needed = snprintf(NULL, 0, __VA_ARGS__);                           \
            Malloc(__n_var, char, needed + 2);                                 \
            if (__n_var) {                                                     \
                snprintf(__n_var, needed + 1, __VA_ARGS__);                    \
            }                                                                  \
        } else {                                                               \
            n_log(LOG_ERR, "%s is already allocated.", "#__n_var");            \
        }                                                                      \
        __n_var;                                                               \
    })

/*! Macro to quickly allocate and sprintf to N_STR  */
#define nstrprintf(__nstr_var, ...)                                            \
    ({                                                                         \
        int needed_size = 0;                                                   \
        needed_size = snprintf(NULL, 0, __VA_ARGS__);                          \
        NSTRBYTE needed = needed_size + 1;                                     \
        if (!__nstr_var || !__nstr_var->data) {                                \
            __nstr_var = new_nstr(needed);                                     \
        } else if (needed > __nstr_var->length) {                              \
            resize_nstr(__nstr_var, needed);                                   \
        }                                                                      \
        snprintf(__nstr_var->data, needed, __VA_ARGS__);                       \
        __nstr_var->written = needed_size;                                     \
        __nstr_var;                                                            \
    })

/*! Macro to quickly allocate and sprintf and cat to a N_STR * */
#define nstrprintf_cat(__nstr_var, ...)                                        \
    ({                                                                         \
        if (__nstr_var) {                                                      \
            int needed_size = 0;                                               \
            needed_size = snprintf(NULL, 0, __VA_ARGS__);                      \
            if (needed_size > 0) {                                             \
                NSTRBYTE needed = needed_size + 1;                             \
                if ((__nstr_var->written + needed) > __nstr_var->length) {     \
                    resize_nstr(__nstr_var, __nstr_var->length + needed);      \
                }                                                              \
                snprintf(__nstr_var->data + __nstr_var->written, needed,       \
                         __VA_ARGS__);                                         \
                __nstr_var->written += needed_size;                            \
            }                                                                  \
        } else {                                                               \
            nstrprintf(__nstr_var, __VA_ARGS__);                               \
        }                                                                      \
        __nstr_var;                                                            \
    })

/*! Remove carriage return (backslash r) if there is one in the last position of
 * the string */
#define n_remove_ending_cr(__nstr_var)                                         \
    if (__nstr_var && __nstr_var->data &&                                      \
        __nstr_var->data[__nstr_var->written] == '\r') {                       \
        __nstr_var->data[__nstr_var->written] = '\0';                          \
        __nstr_var->written--;                                                 \
    }

/*! Find and replace all occurences of carriage return (backslash r) in the
 * string */
#define n_replace_cr(__nstr_var, __replacement)                                \
    if (__nstr_var && __nstr_var->data && __nstr_var->written > 0) {           \
        char *__replaced = str_replace(__nstr_var->data, "\r", __replacement); \
        if (__replaced) {                                                      \
            Free(__nstr_var->data);                                            \
            __nstr_var->data = __replaced;                                     \
            __nstr_var->written = strlen(__nstr_var->data);                    \
            __nstr_var->length = __nstr_var->written + 1;                      \
        }                                                                      \
    }

#include <inttypes.h>
/*! N_STR base unit */
typedef size_t NSTRBYTE;

/*! A box including a string and his lenght */
typedef struct N_STR {
    /*! the string */
    char *data;
    /*! length of string (in case we wanna keep information after the 0 end of
     * string value) */
    size_t length;
    /*! size of the written data inside the string */
    size_t written;
} N_STR;

#ifdef __windows__
const char *strcasestr(const char *s1, const char *s2);
#endif

/**
 * \brief Trim leading and trailing whitespace in-place, setting a null
 * terminator.
 *
 * Modifies s directly.  Returns a pointer to the first non-whitespace character
 * within s (which may be the same pointer as s or further inside it).
 *
 * \param s  String to trim.
 * \return   Pointer to the trimmed start within s.
 */
char *trim_nocopy(char *s);

/**
 * \brief Trim leading and trailing whitespace, returning a new heap string.
 *
 * The caller is responsible for freeing the returned string.
 *
 * \param s  String to trim.
 * \return   Newly allocated trimmed copy, or NULL on allocation failure.
 */
char *trim(char *s);

/**
 * \brief Read a line from a stream into a fixed-size buffer.
 *
 * Thin wrapper around fgets() that returns NULL on EOF or error, and strips a
 * trailing newline if present.
 *
 * \param buffer  Destination buffer.
 * \param size    Size of the buffer in bytes.
 * \param stream  Open FILE stream to read from.
 * \return        buffer on success, NULL on EOF or error.
 */
char *nfgets(char *buffer, NSTRBYTE size, FILE *stream);

/**
 * \brief Allocate a new N_STR with at least size bytes of storage.
 *
 * \param size  Minimum number of bytes to allocate for the string data.
 * \return      Pointer to the new N_STR, or NULL on allocation failure.
 */
N_STR *new_nstr(NSTRBYTE size);

/**
 * \brief Zero the content of an N_STR without freeing it.
 *
 * \param nstr  N_STR to reinitialise.
 * \return      TRUE on success, FALSE if nstr is NULL.
 */
int empty_nstr(N_STR *nstr);

/**
 * \brief Make a deep copy of an N_STR.
 *
 * \param msg  Source N_STR to duplicate.
 * \return     New N_STR containing a copy of msg, or NULL on failure.
 */
N_STR *nstrdup(N_STR *msg);

/**
 * \brief Wrap nboct bytes of a char buffer in an N_STR (copying data).
 *
 * \param from   Source byte buffer.
 * \param nboct  Number of bytes to copy from from.
 * \param to     Receives the new N_STR pointer.
 * \return       TRUE on success, FALSE on failure.
 */
int char_to_nstr_ex(const char *from, NSTRBYTE nboct, N_STR **to);

/**
 * \brief Wrap a null-terminated char string in a new N_STR (copying data).
 *
 * \param src  Source null-terminated string.
 * \return     New N_STR, or NULL on failure.
 */
N_STR *char_to_nstr(const char *src);

/**
 * \brief Wrap nboct bytes of a char buffer in an N_STR without copying.
 *
 * The N_STR takes ownership of the from pointer.  Do not free from directly
 * after this call.
 *
 * \param from   Source byte buffer (ownership transferred).
 * \param nboct  Number of bytes pointed to by from.
 * \param to     Receives the new N_STR pointer.
 * \return       TRUE on success, FALSE on failure.
 */
int char_to_nstr_nocopy_ex(char *from, NSTRBYTE nboct, N_STR **to);

/**
 * \brief Wrap a null-terminated string in an N_STR without copying.
 *
 * \param src  Source null-terminated string (ownership transferred).
 * \return     New N_STR, or NULL on failure.
 */
N_STR *char_to_nstr_nocopy(char *src);

/**
 * \brief Append raw bytes to an N_STR, optionally resizing it.
 *
 * \param dest         Destination N_STR.
 * \param src          Source byte buffer.
 * \param size         Number of bytes to append.
 * \param resize_flag  Non-zero to allow automatic resize; 0 to fail on
 * overflow.
 * \return             TRUE on success, FALSE on failure.
 */
int nstrcat_ex(N_STR *dest, void *src, NSTRBYTE size, int resize_flag);

/**
 * \brief Concatenate two N_STR values.
 *
 * \param dst  Destination N_STR; src is appended to it.
 * \param src  Source N_STR to append.
 * \return     TRUE on success, FALSE on failure.
 */
int nstrcat(N_STR *dst, N_STR *src);

/**
 * \brief Append a raw byte buffer of known size to an N_STR.
 *
 * \param dest  Destination N_STR.
 * \param src   Source byte buffer.
 * \param size  Number of bytes to append.
 * \return      TRUE on success, FALSE on failure.
 */
int nstrcat_bytes_ex(N_STR *dest, void *src, NSTRBYTE size);

/**
 * \brief Append a null-terminated byte buffer to an N_STR.
 *
 * \param dest  Destination N_STR.
 * \param src   Null-terminated source buffer.
 * \return      TRUE on success, FALSE on failure.
 */
int nstrcat_bytes(N_STR *dest, void *src);

/**
 * \brief Load an entire file into a new N_STR.
 *
 * Limited to files of at most min(4 GB, available system memory) in size.
 *
 * \param filename  Path of the file to load.
 * \return          New N_STR containing the file contents, or NULL on failure.
 */
N_STR *file_to_nstr(char *filename);

/**
 * \brief Write the full contents of an N_STR to an open FILE descriptor.
 *
 * \param str   N_STR to write.
 * \param out   Open FILE pointer to write to.
 * \param lock  Non-zero to acquire a POSIX file lock before writing.
 * \return      TRUE on success, FALSE on failure.
 */
int nstr_to_fd(N_STR *str, FILE *out, int lock);

/**
 * \brief Write the full contents of an N_STR to a file by name.
 *
 * \param n_str     N_STR to write.
 * \param filename  Destination file path.
 * \return          TRUE on success, FALSE on failure.
 */
int nstr_to_file(N_STR *n_str, char *filename);

/*! free a N_STR structure and set the pointer to NULL */
#define free_nstr(__ptr)                                                       \
    {                                                                          \
        if ((*__ptr)) {                                                        \
            _free_nstr(__ptr);                                                 \
        } else {                                                               \
            n_log(LOG_DEBUG, "%s is already NULL", #__ptr);                    \
        }                                                                      \
    }

/**
 * \brief Free an N_STR and set its pointer to NULL.  Called by free_nstr()
 * macro.
 *
 * \param ptr  Address of the N_STR pointer to free.
 * \return     TRUE on success.
 */
int _free_nstr(N_STR **ptr);

/**
 * \brief Free an N_STR passed as void * (for use as a list destructor).
 *
 * \param ptr  N_STR pointer cast to void *.
 */
void free_nstr_ptr(void *ptr);

/**
 * \brief Free an N_STR and set its pointer to NULL without logging.
 *
 * \param ptr  Address of the N_STR pointer to free.
 * \return     TRUE on success.
 */
int free_nstr_nolog(N_STR **ptr);

/**
 * \brief Free an N_STR passed as void * without logging.
 *
 * \param ptr  N_STR pointer cast to void *.
 */
void free_nstr_ptr_nolog(void *ptr);

/**
 * \brief Convert a substring of s (bytes [start, end)) to a long integer.
 *
 * \param s     Source string.
 * \param start Start index (inclusive).
 * \param end   End index (exclusive).
 * \param i     Receives the parsed value.
 * \param base  Numeric base (e.g. 10, 16).
 * \return      TRUE on success, FALSE on parse error.
 */
int str_to_long_ex(const char *s, NSTRBYTE start, NSTRBYTE end, long int *i,
                   const int base);

/**
 * \brief Convert a null-terminated string to a long integer.
 *
 * \param s     Source string.
 * \param i     Receives the parsed value.
 * \param base  Numeric base.
 * \return      TRUE on success, FALSE on parse error.
 */
int str_to_long(const char *s, long int *i, const int base);

/**
 * \brief Convert a substring of s to a long long integer.
 *
 * \param s     Source string.
 * \param start Start index (inclusive).
 * \param end   End index (exclusive).
 * \param i     Receives the parsed value.
 * \param base  Numeric base.
 * \return      TRUE on success, FALSE on parse error.
 */
int str_to_long_long_ex(const char *s, NSTRBYTE start, NSTRBYTE end,
                        long long int *i, const int base);

/**
 * \brief Convert a null-terminated string to a long long integer.
 *
 * \param s     Source string.
 * \param i     Receives the parsed value.
 * \param base  Numeric base.
 * \return      TRUE on success, FALSE on parse error.
 */
int str_to_long_long(const char *s, long long int *i, const int base);

/**
 * \brief Convert a substring of s to an integer with error checking.
 *
 * \param s     Source string.
 * \param start Start index (inclusive).
 * \param end   End index (exclusive).
 * \param i     Receives the parsed value.
 * \param base  Numeric base.
 * \return      TRUE on success, FALSE on parse error.
 */
int str_to_int_ex(const char *s, NSTRBYTE start, NSTRBYTE end, int *i,
                  const int base);

/**
 * \brief Convert a substring of s to an integer, storing error info in infos.
 *
 * Does not log errors; instead stores a diagnostic message in *infos.
 *
 * \param s     Source string.
 * \param start Start index (inclusive).
 * \param end   End index (exclusive).
 * \param i     Receives the parsed value.
 * \param base  Numeric base.
 * \param infos Receives a diagnostic N_STR on parse error; may be NULL.
 * \return      TRUE on success, FALSE on parse error.
 */
int str_to_int_nolog(const char *s, NSTRBYTE start, NSTRBYTE end, int *i,
                     const int base, N_STR **infos);

/**
 * \brief Convert a null-terminated string to an integer.
 *
 * \param s     Source string.
 * \param i     Receives the parsed value.
 * \param base  Numeric base.
 * \return      TRUE on success, FALSE on parse error.
 */
int str_to_int(const char *s, int *i, const int base);

/**
 * \brief Advance an iterator while string[iterator] == toskip.
 *
 * \param string    String to scan.
 * \param toskip    Character to skip over.
 * \param iterator  Current position; updated in place.
 * \param inc       Increment per step (+1 = forward, -1 = backward).
 * \return          TRUE if the iterator was advanced at least once.
 */
int skipw(char *string, char toskip, NSTRBYTE *iterator, int inc);

/**
 * \brief Advance an iterator until string[iterator] == toskip.
 *
 * \param string    String to scan.
 * \param toskip    Stop character.
 * \param iterator  Current position; updated in place.
 * \param inc       Increment per step.
 * \return          TRUE if the stop character was found.
 */
int skipu(char *string, char toskip, NSTRBYTE *iterator, int inc);

/**
 * \brief Convert string to upper case, writing the result to dest.
 *
 * \param string  Source string.
 * \param dest    Destination buffer (must be at least as large as string).
 * \return        TRUE on success, FALSE if arguments are NULL.
 */
int strup(char *string, char *dest);

/**
 * \brief Convert string to lower case, writing the result to dest.
 *
 * \param string  Source string.
 * \param dest    Destination buffer (must be at least as large as string).
 * \return        TRUE on success, FALSE if arguments are NULL.
 */
int strlo(char *string, char *dest);

/**
 * \brief Copy bytes from from to to, stopping at the split delimiter.
 *
 * \param from     Source string.
 * \param to       Destination buffer.
 * \param to_size  Size of the destination buffer.
 * \param split    Delimiter character that terminates the copy.
 * \param it       Iterator position in from; updated to the position after
 * split.
 * \return         Number of bytes copied.
 */
int strcpy_u(char *from, char *to, NSTRBYTE to_size, char split, NSTRBYTE *it);

/**
 * \brief Split a string by a delimiter and return a NULL-terminated array.
 *
 * \param str    String to split.
 * \param delim  Delimiter string.
 * \param empty  Non-zero to include empty fields; 0 to skip them.
 * \return       Heap-allocated NULL-terminated array of strings, or NULL.
 *               Free with free_split_result().
 */
char **split(const char *str, const char *delim, int empty);

/**
 * \brief Count the number of entries in a split result array.
 *
 * \param split_result  NULL-terminated array returned by split().
 * \return              Number of non-NULL entries.
 */
int split_count(char **split_result);

/**
 * \brief Free a split result array and set the pointer to NULL.
 *
 * \param tab  Address of the split result pointer to free.
 * \return     TRUE on success.
 */
int free_split_result(char ***tab);

/**
 * \brief Join a split result array into a single string with a delimiter.
 *
 * \param splitresult  NULL-terminated array of strings to join.
 * \param delim        Delimiter string placed between elements.
 * \return             Newly allocated joined string, or NULL on failure.
 */
char *join(char **splitresult, char *delim);

/**
 * \brief Append src_size bytes of src into a dynamically resized char buffer.
 *
 * Reallocates *dest if needed to fit the new data.
 *
 * \param dest               Address of the destination buffer pointer.
 * \param size               Current allocated size of *dest; updated on resize.
 * \param written            Number of bytes already written; updated after
 * append.
 * \param src                Source byte buffer.
 * \param src_size           Number of bytes to append.
 * \param additional_padding Extra bytes to allocate beyond the required size.
 * \return                   TRUE on success, FALSE on allocation failure.
 */
int write_and_fit_ex(char **dest, NSTRBYTE *size, NSTRBYTE *written,
                     const char *src, NSTRBYTE src_size,
                     NSTRBYTE additional_padding);

/**
 * \brief Append a null-terminated string to a dynamically resized char buffer.
 *
 * \param dest     Address of the destination buffer pointer.
 * \param size     Current allocated size of *dest; updated on resize.
 * \param written  Number of bytes already written; updated after append.
 * \param src      Null-terminated source string.
 * \return         TRUE on success, FALSE on allocation failure.
 */
int write_and_fit(char **dest, NSTRBYTE *size, NSTRBYTE *written,
                  const char *src);

/**
 * \brief Scan a directory and append file paths to a LIST.
 *
 * \param dir      Directory path to scan.
 * \param result   LIST to append file paths (as char *) to.
 * \param recurse  Non-zero to recurse into subdirectories.
 * \return         Number of entries added, or -1 on error.
 */
int scan_dir(const char *dir, LIST *result, const int recurse);

/**
 * \brief Scan a directory, filtering by pattern, and append N_STR paths to a
 * LIST.
 *
 * \param dir      Directory path to scan.
 * \param pattern  Wildmat pattern to match file names against.
 * \param result   LIST to append N_STR file paths to.
 * \param recurse  Non-zero to recurse into subdirectories.
 * \param mode     Reserved for future use; pass 0.
 * \return         Number of entries added, or -1 on error.
 */
int scan_dir_ex(const char *dir, const char *pattern, LIST *result,
                const int recurse, const int mode);

/**
 * \brief Test whether text matches a wildmat glob pattern (case-sensitive).
 *
 * Supports *, ? and character class [] patterns.
 *
 * \param text  String to match.
 * \param p     Wildmat pattern.
 * \return      1 if text matches p, 0 otherwise.
 */
int wildmat(register const char *text, register const char *p);

/**
 * \brief Test whether text matches a wildmat glob pattern (case-insensitive).
 *
 * \param text  String to match.
 * \param p     Wildmat pattern.
 * \return      1 if text matches p, 0 otherwise.
 */
int wildmatcase(register const char *text, register const char *p);

/**
 * \brief Return a new string with every occurrence of substr replaced.
 *
 * \param string       Source string.
 * \param substr       Substring to find.
 * \param replacement  Replacement string.
 * \return             Newly allocated string with substitutions applied, or
 * NULL.
 */
char *str_replace(const char *string, const char *substr,
                  const char *replacement);

/**
 * \brief Replace all characters in mask with replacement, operating on a
 * substring.
 *
 * \param string      String to sanitize (modified in place).
 * \param string_len  Length of string to process.
 * \param mask        Set of characters to replace.
 * \param masklen     Length of the mask string.
 * \param replacement Single replacement character.
 * \return            Number of substitutions made, or -1 on error.
 */
int str_sanitize_ex(char *string, const NSTRBYTE string_len, const char *mask,
                    const NSTRBYTE masklen, const char replacement);

/**
 * \brief Replace all characters in mask with replacement in a null-terminated
 * string.
 *
 * \param string      String to sanitize (modified in place).
 * \param mask        Set of characters to replace.
 * \param replacement Single replacement character.
 * \return            Number of substitutions made, or -1 on error.
 */
int str_sanitize(char *string, const char *mask, const char replacement);

/**
@}
*/

#ifdef __cplusplus
}
#endif
/* #ifndef N_STR*/
#endif
