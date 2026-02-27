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

/**\file n_log.h
 * Generic log system
 *\author Castagnier Mickael
 *\date 2013-04-15
 */

#ifndef __LOG_HEADER_GUARD__
#define __LOG_HEADER_GUARD__

#ifdef __cplusplus
extern "C" {
#endif

/**\defgroup LOG LOGGING: logging to console, to file, to syslog, to event log
  \addtogroup LOG
  @{
  */

#include "n_common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/*! no log output */
#define LOG_NULL -1
/*! internal, logging to file */
#define LOG_FILE -3
/*! internal, default LOG_TYPE */
#define LOG_STDERR -4
/*! to sysjrnl */
#define LOG_SYSJRNL 100

#if defined(__linux__) || defined(__sun)

#include <pthread.h>
#include <syslog.h>

#ifdef __sun
#include <sys/varargs.h>
#else
#include <stdarg.h>
#endif

#else

#include <stdarg.h>

/*! system is unusable */
#define LOG_EMERG 0
/*! action must be taken immediately */
#define LOG_ALERT 1
/*! critical conditions */
#define LOG_CRIT 2
/*! error conditions */
#define LOG_ERR 3
/*! warning conditions */
#define LOG_WARNING 4
/*! normal but significant condition */
#define LOG_NOTICE 5
/*! informational */
#define LOG_INFO 6
/*! debug-level messages */
#define LOG_DEBUG 7

#endif

/*! Logging function wrapper to get line and func */
#define n_log(__LEVEL__, ...)                                                  \
    do {                                                                       \
        _n_log(__LEVEL__, __FILE__, __func__, __LINE__, __VA_ARGS__);          \
    } while (0)

/*! ThreadSafe LOGging structure */
typedef struct TS_LOG {
    /*! mutex for thread-safe writting */
    pthread_mutex_t LOG_MUTEX;
    /*! File handler */
    FILE *file;
} TS_LOG;

/**
 * \brief Open the system journal (syslog / Windows Event Log).
 *
 * \param identity  Program name used as the log identity string.
 * \return          The identity string, or NULL on failure.
 */
char *open_sysjrnl(char *identity);

/**
 * \brief Close the system journal opened by open_sysjrnl().
 */
void close_sysjrnl(void);

/**
 * \brief Set the global log level.
 *
 * Messages with a severity higher (numerically greater) than log_level are
 * suppressed.  Use LOG_NULL to suppress all output.
 *
 * \param log_level  One of LOG_EMERG .. LOG_DEBUG, LOG_NULL, or LOG_SYSJRNL.
 */
void set_log_level(const int log_level);

/**
 * \brief Return the current global log level.
 *
 * \return  Current log level as set by set_log_level().
 */
int get_log_level(void);

/**
 * \brief Redirect log output to a file opened by name.
 *
 * \param file  Path of the file to open for writing.
 * \return      TRUE on success, FALSE on failure.
 */
int set_log_file(char *file);

/**
 * \brief Redirect log output to an already-open FILE descriptor.
 *
 * \param fd  Open FILE pointer to write log messages to.
 * \return    TRUE on success, FALSE on failure.
 */
int set_log_file_fd(FILE *fd);

/**
 * \brief Return the current log output FILE pointer.
 *
 * \return  The FILE pointer currently used for log output, or NULL.
 */
FILE *get_log_file(void);

/**
 * \brief Core logging function.  Use the n_log() macro instead of calling
 * directly.
 *
 * The n_log() macro wraps this function and passes the caller's
 * __FILE__, __func__ and __LINE__ automatically.
 *
 * \param level   Severity level (LOG_EMERG .. LOG_DEBUG).
 * \param file    Source file name of the call site.
 * \param func    Function name of the call site.
 * \param line    Line number of the call site.
 * \param format  printf-style format string.
 * \param ...     Format arguments.
 */
void _n_log(int level, const char *file, const char *func, int line,
            const char *format, ...);

/**
 * \brief Open a thread-safe log file.
 *
 * \param log       Receives the allocated TS_LOG pointer.
 * \param pathname  Path of the log file to create or append to.
 * \param opt       fopen mode string (e.g. "a", "w").
 * \return          TRUE on success, FALSE on failure.
 */
int open_safe_logging(TS_LOG **log, char *pathname, char *opt);

/**
 * \brief Write a formatted message to a thread-safe log file.
 *
 * The write is serialised with the TS_LOG mutex so that concurrent threads
 * do not interleave their output.
 *
 * \param log  TS_LOG handle returned by open_safe_logging().
 * \param pat  printf-style format string.
 * \param ...  Format arguments.
 * \return     TRUE on success, FALSE on failure.
 */
int write_safe_log(TS_LOG *log, char *pat, ...);

/**
 * \brief Close a thread-safe log file and free its resources.
 *
 * \param log  TS_LOG handle to close and free.
 * \return     TRUE on success, FALSE on failure.
 */
int close_safe_logging(TS_LOG *log);

/**
@}
*/

#ifdef __cplusplus
}
#endif
#endif
