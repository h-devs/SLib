/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#ifndef CHECKHEADER_SLIB_CORE_DL_LIBMYSQL
#define CHECKHEADER_SLIB_CORE_DL_LIBMYSQL

#include "../core/definition.h"

#if defined(SLIB_PLATFORM_IS_DESKTOP)

#include "../core/dl.h"

#include "libmariadb/mysql.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(libmysql, "libmysql.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_init,
			MYSQL_STMT *, STDCALL,
			MYSQL *mysql
		)
		#define mysql_stmt_init slib::libmysql::getApi_mysql_stmt_init()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_prepare,
			int, STDCALL,
			MYSQL_STMT *stmt, const char *query, unsigned long length
		)
		#define mysql_stmt_prepare slib::libmysql::getApi_mysql_stmt_prepare()
	
		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_execute,
			int, STDCALL,
			MYSQL_STMT *stmt
		)
		#define mysql_stmt_execute slib::libmysql::getApi_mysql_stmt_execute()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_fetch,
			int, STDCALL,
			MYSQL_STMT *stmt
		)
		#define mysql_stmt_fetch slib::libmysql::getApi_mysql_stmt_fetch()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_fetch_column,
			int, STDCALL,
			MYSQL_STMT *stmt, MYSQL_BIND *bind_arg, unsigned int column, unsigned long offset
		)
		#define mysql_stmt_fetch_column slib::libmysql::getApi_mysql_stmt_fetch_column()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_param_count,
			unsigned long, STDCALL,
			MYSQL_STMT *stmt
		)
		#define mysql_stmt_param_count slib::libmysql::getApi_mysql_stmt_param_count()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_bind_param,
			my_bool, STDCALL,
			MYSQL_STMT * stmt, MYSQL_BIND * bnd
		)
		#define mysql_stmt_bind_param slib::libmysql::getApi_mysql_stmt_bind_param()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_bind_result,
			my_bool, STDCALL,
			MYSQL_STMT * stmt, MYSQL_BIND * bnd
		)
		#define mysql_stmt_bind_result slib::libmysql::getApi_mysql_stmt_bind_result()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_close,
			my_bool, STDCALL,
			MYSQL_STMT * stmt
		)
		#define mysql_stmt_close slib::libmysql::getApi_mysql_stmt_close()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_free_result,
			my_bool, STDCALL,
			MYSQL_STMT * stmt
		)
		#define mysql_stmt_free_result slib::libmysql::getApi_mysql_stmt_free_result()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_result_metadata,
			MYSQL_RES *, STDCALL,
			MYSQL_STMT * stmt
		)
		#define mysql_stmt_result_metadata slib::libmysql::getApi_mysql_stmt_result_metadata()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_errno,
			unsigned int, STDCALL,
			MYSQL_STMT * stmt
		)
		#define mysql_stmt_errno slib::libmysql::getApi_mysql_stmt_errno()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_error,
			const char *, STDCALL,
			MYSQL_STMT * stmt
		)
		#define mysql_stmt_error slib::libmysql::getApi_mysql_stmt_error()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_num_rows,
			unsigned long long, STDCALL,
			MYSQL_STMT * stmt
		)
		#define mysql_stmt_num_rows slib::libmysql::getApi_mysql_stmt_num_rows()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_affected_rows,
			unsigned long long, STDCALL,
			MYSQL_STMT * stmt
		)
		#define mysql_stmt_affected_rows slib::libmysql::getApi_mysql_stmt_affected_rows()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_stmt_insert_id,
			unsigned long long, STDCALL,
			MYSQL_STMT * stmt
		)
		#define mysql_stmt_insert_id slib::libmysql::getApi_mysql_stmt_insert_id()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_num_rows,
			my_ulonglong, STDCALL,
			MYSQL_RES *res
		)
		#define mysql_num_rows slib::libmysql::getApi_mysql_num_rows()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_num_fields,
			unsigned int, STDCALL,
			MYSQL_RES *res
		)
		#define mysql_num_fields slib::libmysql::getApi_mysql_num_fields()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_eof,
			my_bool, STDCALL,
			MYSQL_RES *res
		)
		#define mysql_eof slib::libmysql::getApi_mysql_eof()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_fetch_fields,
			MYSQL_FIELD *, STDCALL,
			MYSQL_RES *res
		)
		#define mysql_fetch_fields slib::libmysql::getApi_mysql_fetch_fields()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_field_count,
			unsigned int, STDCALL,
			MYSQL *mysql
		)
		#define mysql_field_count slib::libmysql::getApi_mysql_field_count()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_next_result,
			int, STDCALL,
			MYSQL *mysql
		)
		#define mysql_next_result slib::libmysql::getApi_mysql_next_result()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_affected_rows,
			my_ulonglong, STDCALL,
			MYSQL *mysql
		)
		#define mysql_affected_rows slib::libmysql::getApi_mysql_affected_rows()
		
		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_autocommit,
			my_bool, STDCALL,
			MYSQL *mysql, my_bool mode
		)
		#define mysql_autocommit slib::libmysql::getApi_mysql_autocommit()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_insert_id,
			my_ulonglong, STDCALL,
			MYSQL *mysql
		)
		#define mysql_insert_id slib::libmysql::getApi_mysql_insert_id()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_errno,
			unsigned int, STDCALL,
			MYSQL *mysql
		)
		#define mysql_errno slib::libmysql::getApi_mysql_errno()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_error,
			const char *, STDCALL,
			MYSQL *mysql
		)
		#define mysql_error slib::libmysql::getApi_mysql_error()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_set_character_set,
			int, STDCALL,
			MYSQL *mysql, const char *csname
		)
		#define mysql_set_character_set slib::libmysql::getApi_mysql_set_character_set()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_init,
			MYSQL *, STDCALL,
			MYSQL *mysql
		)
		#define mysql_init slib::libmysql::getApi_mysql_init()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_real_connect,
			MYSQL *, STDCALL,
			MYSQL *mysql,
			const char *host,
			const char *user,
			const char *passwd,
			const char *db,
			unsigned int port,
			const char *unix_socket,
			unsigned long clientflag
		)
		#define mysql_real_connect slib::libmysql::getApi_mysql_real_connect()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_close,
			void, STDCALL,
			MYSQL *mysql
		)
		#define mysql_close slib::libmysql::getApi_mysql_close()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_real_query,
			int, STDCALL,
			MYSQL *mysql, const char *q, unsigned long length
		)
		#define mysql_real_query slib::libmysql::getApi_mysql_real_query()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_ping,
			int, STDCALL,
			MYSQL *mysql
		)
		#define mysql_ping slib::libmysql::getApi_mysql_ping()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_list_dbs,
			MYSQL_RES *, STDCALL,
			MYSQL *mysql, const char *wild
		)
		#define mysql_list_dbs slib::libmysql::getApi_mysql_list_dbs()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_list_tables,
			MYSQL_RES *, STDCALL,
			MYSQL *mysql, const char *wild
		)
		#define mysql_list_tables slib::libmysql::getApi_mysql_list_tables()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_list_fields,
			MYSQL_RES *, STDCALL,
			MYSQL *mysql, const char *table, const char *wild
		)
		#define mysql_list_fields slib::libmysql::getApi_mysql_list_fields()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_store_result,
			MYSQL_RES *, STDCALL,
			MYSQL *mysql
		)
		#define mysql_store_result slib::libmysql::getApi_mysql_store_result()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_use_result,
			MYSQL_RES *, STDCALL,
			MYSQL *mysql
		)
		#define mysql_use_result slib::libmysql::getApi_mysql_use_result()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_options,
			int, STDCALL,
			MYSQL *mysql, enum mysql_option option, const void *arg
		)
		#define mysql_options slib::libmysql::getApi_mysql_options()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_free_result,
			void, STDCALL,
			MYSQL_RES *result
		)
		#define mysql_free_result slib::libmysql::getApi_mysql_free_result()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_fetch_row,
			MYSQL_ROW, STDCALL,
			MYSQL_RES *result
		)
		#define mysql_fetch_row slib::libmysql::getApi_mysql_fetch_row()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_fetch_lengths,
			unsigned long *, STDCALL,
			MYSQL_RES *result
		)
		#define mysql_fetch_lengths slib::libmysql::getApi_mysql_fetch_lengths()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_server_init,
			int, STDCALL,
			int argc, char **argv, char **groups
		)
		#define mysql_server_init slib::libmysql::getApi_mysql_server_init()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_thread_end,
			void, STDCALL,
			void
		)
		#define mysql_thread_end slib::libmysql::getApi_mysql_thread_end()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			mysql_thread_init,
			my_bool, STDCALL,
			void
		)
		#define mysql_thread_init slib::libmysql::getApi_mysql_thread_init()

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
