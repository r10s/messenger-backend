/*******************************************************************************
 *
 *                              Delta Chat Core
 *                      Copyright (C) 2017 Björn Petersen
 *                   Contact: r10s@b44t.com, http://b44t.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see http://www.gnu.org/licenses/ .
 *
 ******************************************************************************/


#ifndef __DC_SQLITE3_H__
#define __DC_SQLITE3_H__
#ifdef __cplusplus
extern "C" {
#endif


/*** library-private **********************************************************/

#include <sqlite3.h>
#include <libetpan/libetpan.h>
#include <pthread.h>


/* predefined statements */
enum
{
	 BEGIN_transaction = 0 /* must be first */
	,ROLLBACK_transaction
	,COMMIT_transaction

	,SELECT_id_FROM_CHATS_WHERE_grpid
	,SELECT_timestamp_FROM_msgs_WHERE_timestamp
	,SELECT_param_FROM_msgs
	,UPDATE_chats_SET_blocked_WHERE_chat_id

	,SELECT_a_FROM_chats_contacts_WHERE_i
	,SELECT_verified_FROM_chats_contacts_WHERE_chat_id
	,SELECT_contact_id_FROM_chats_contacts_WHERE_chat_id_ORDER_BY
	,INSERT_INTO_chats_contacts

	,SELECT_i_FROM_msgs_WHERE_ctt
	,SELECT_id_FROM_msgs_WHERE_cm
	,SELECT_id_FROM_msgs_WHERE_mcm
	,SELECT_id_FROM_msgs_WHERE_fresh_AND_deaddrop
	,SELECT_txt_raw_FROM_msgs_WHERE_id
	,SELECT_ss_FROM_msgs_WHERE_m
	,SELECT_i_FROM_msgs_LEFT_JOIN_contacts_WHERE_c
	,SELECT_i_FROM_msgs_LEFT_JOIN_contacts_WHERE_starred
	,SELECT_i_FROM_msgs_LEFT_JOIN_contacts_WHERE_fresh
	,SELECT_i_FROM_msgs_LEFT_JOIN_chats_contacts_WHERE_blocked
	,SELECT_i_FROM_msgs_WHERE_query
	,SELECT_i_FROM_msgs_WHERE_chat_id_AND_query

	,PREDEFINED_CNT /* must be last */
};


/**
 * Library-internal.
 *
 * In general, function names ending with two underscores (`__`) implie that _no_
 * locking takes place inside the functions!  So the caller must make sure, the
 * database is locked as needed.  Of course, the same is true if you call any
 * sqlite3-function directly.
 */
typedef struct dc_sqlite3_t
{
	/** @privatesection */
	sqlite3_stmt*   m_pd[PREDEFINED_CNT]; /**< prepared statements - this is the favourite way for the caller to use SQLite */
	sqlite3*        m_cobj;               /**< is the database given as dbfile to Open() */
	int             m_transactionCount;   /**< helper for transactions */
	dc_context_t*   m_context;            /**< used for logging and to acquire wakelocks, there may be N dc_sqlite3_t objects per context! In practise, we use 2 on backup, 1 otherwise. */
	pthread_mutex_t m_critical_;        /**< the user must make sure, only one thread uses sqlite at the same time! for this purpose, all calls must be enclosed by a locked m_critical; use dc_sqlite3_lock() for this purpose */

} dc_sqlite3_t;


dc_sqlite3_t* dc_sqlite3_new              (dc_context_t*);
void          dc_sqlite3_unref            (dc_sqlite3_t*);

#define       DC_OPEN_READONLY           0x01
int           dc_sqlite3_open__           (dc_sqlite3_t*, const char* dbfile, int flags);

void          dc_sqlite3_close__          (dc_sqlite3_t*);
int           dc_sqlite3_is_open          (const dc_sqlite3_t*);

/* handle configurations, private */
int           dc_sqlite3_set_config       (dc_sqlite3_t*, const char* key, const char* value);
int           dc_sqlite3_set_config_int   (dc_sqlite3_t*, const char* key, int32_t value);
char*         dc_sqlite3_get_config       (dc_sqlite3_t*, const char* key, const char* def); /* the returned string must be free()'d, returns NULL on errors */
int32_t       dc_sqlite3_get_config_int   (dc_sqlite3_t*, const char* key, int32_t def);

/* tools, these functions are compatible to the corresponding sqlite3_* functions */
sqlite3_stmt* dc_sqlite3_predefine__      (dc_sqlite3_t*, size_t idx, const char* sql); /*the result is resetted as needed and must not be freed. CAVE: you must not call this function with different strings for the same index!*/
sqlite3_stmt* dc_sqlite3_prepare          (dc_sqlite3_t*, const char* sql); /* the result mus be freed using sqlite3_finalize() */
int           dc_sqlite3_execute          (dc_sqlite3_t*, const char* sql);
int           dc_sqlite3_table_exists__   (dc_sqlite3_t*, const char* name);
void          dc_sqlite3_log_error        (dc_sqlite3_t*, const char* msg, ...);

/* reset all predefined statements, this is needed only in very rare cases, eg. when dropping a table and there are pending statements */
void          dc_sqlite3_reset_all_predefinitions(dc_sqlite3_t*);

/* tools for locking, may be called nested, see also m_critical_ above.
the user of dc_sqlite3 must make sure that the dc_sqlite3-object is only used by one thread at the same time.
In general, we will lock the hightest level as possible - this avoids deadlocks and massive on/off lockings.
Low-level-functions, eg. the dc_sqlite3-methods, do not lock. */
#ifdef DC_USE_LOCK_DEBUG
#define       dc_sqlite3_lock(a)          dc_sqlite3_lockNdebug((a), __FILE__, __LINE__)
#define       dc_sqlite3_unlock(a)        dc_sqlite3_unlockNdebug((a), __FILE__, __LINE__)
void          dc_sqlite3_lockNdebug       (dc_sqlite3_t*, const char* filename, int line);
void          dc_sqlite3_unlockNdebug     (dc_sqlite3_t*, const char* filename, int line);
#else
void          dc_sqlite3_lock             (dc_sqlite3_t*); /* lock or wait; these calls must not be nested in a single thread */
void          dc_sqlite3_unlock           (dc_sqlite3_t*);
#endif

/* nestable transactions, only the outest is really used */
void          dc_sqlite3_begin_transaction__(dc_sqlite3_t*);
void          dc_sqlite3_commit__           (dc_sqlite3_t*);
void          dc_sqlite3_rollback__         (dc_sqlite3_t*);

#ifdef __cplusplus
} /* /extern "C" */
#endif
#endif /* __DC_SQLITE3_H__ */
