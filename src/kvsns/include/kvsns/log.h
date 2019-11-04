/*
 * log.h
 * Header file for KVSNS logging interfaces
 */

#ifndef KVSNS_LOG_H
#define KVSNS_LOG_H

#include <stdio.h>

#ifndef DNOT_TRACE
#include <lib/trace.h>

/* @todo : Improve this simplistic implementation of logging. */
#define KVSNS_LOG(fmt, ...) fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#define KVSNS_LOG_ERR(fmt, ...) M0_LOG(M0_ERROR, fmt, ##__VA_ARGS__)
#define KVSNS_LOG_WARN(fmt, ...) M0_LOG(M0_WARN, fmt, ##__VA_ARGS__)
#define KVSNS_LOG_INFO(fmt, ...) M0_LOG(M0_INFO, fmt, ##__VA_ARGS__)
#define KVSNS_LOG_DEBUG(fmt, ...) M0_LOG(M0_DEBUG, fmt, ##__VA_ARGS__) 
#define KVSNS_LOG_TRACE(fmt, ...) M0_LOG(M0_DEBUG, fmt, ##__VA_ARGS__) /* Need to fix Log Level */

/* @todo: Add more logging levels. */
#define log_err KVSNS_LOG_ERR
#define log_warn KVSNS_LOG_WARN
#define log_info KVSNS_LOG_INFO
#define log_debug KVSNS_LOG
#define log_trace KVSNS_LOG_TRACE

#else
#define KVSNS_LOG(fmt, ...) fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__)

#define log_err KVSNS_LOG
#define log_warn KVSNS_LOG
#define log_info KVSNS_LOG
#define log_debug KVSNS_LOG
#define log_trace KVSNS_LOG

#endif

#endif
