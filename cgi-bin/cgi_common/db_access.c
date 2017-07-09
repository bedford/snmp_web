#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "db_access.h"
#include "sqlite3.h"

typedef struct
{
    char        db_filename[256];
    sqlite3     *db;
} priv_info;

/**
 * @brief   execute_no_query 非数据查询类的操作调用接口
 *
 * @param   thiz
 * @param   sql
 * @param   err_msg
 *
 * @return
 */
static int execute_no_query(db_access_t *thiz, const char *sql, char *err_msg)
{
    priv_info *priv = (priv_info *)thiz->priv;
    sqlite3_stmt *stmt = NULL;
    sqlite3_exec(priv->db, "begin", 0, 0, 0);

    if (sqlite3_prepare_v2(priv->db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK) {
        if (stmt != NULL) {
            sqlite3_finalize(stmt);
        }

		memcpy(err_msg, sqlite3_errmsg(priv->db), 256);
        fprintf(stderr, "errmsg:%s, sql %s\n", sqlite3_errmsg(priv->db), sql);
        return -1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
		memcpy(err_msg, sqlite3_errmsg(priv->db), 256);
        fprintf(stderr, "errmsg:%s, sql %s\n", sqlite3_errmsg(priv->db), sql);
        return -1;
    }

    if (stmt != NULL) {
        sqlite3_finalize(stmt);
    }
    sqlite3_exec(priv->db, "commit", 0, 0, 0);

    return 0;
}

/**
 * @brief   execute_query 数据库查询调用接口
 *
 * @param   thiz
 * @param   sql
 * @param   query_result
 *
 * @return
 */
static int execute_query(db_access_t *thiz, const char *sql, query_result_t *query_result)
{
	priv_info *priv = (priv_info *)thiz->priv;
	char *error_msg = NULL;
	int ret = sqlite3_get_table(priv->db, sql, &(query_result->result), &(query_result->row),
								&(query_result->column), &error_msg);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "error_msg:%s, sql %s\n", error_msg, sql);
		sqlite3_free(error_msg);
		return -1;
	}

	return 0;
}

/**
 * @brief   db_free_table 释放查询返回数据资源
 *
 * @param   thiz
 * @param   result
 *
 * @return
 */
static int db_free_table(db_access_t *thiz, char **result)
{
	sqlite3_free_table(result);

	return 0;
}

/**
 * @brief   db_access_destroy 释放数据库访问对象资源
 *
 * @param   thiz
 */
static void db_access_destroy(db_access_t *thiz)
{
    if (thiz != NULL) {
        priv_info *priv = (priv_info *)thiz->priv;
        if (priv->db) {
            sqlite3_close(priv->db);
            priv->db = NULL;
        }

        memset(thiz, 0, sizeof(db_access_t) + sizeof(priv_info));
        free(thiz);
        thiz = NULL;
    }
}

/**
 * @brief   db_access_create 创建访问数据库对象
 *
 * @param   file_name
 *
 * @return
 */
db_access_t *db_access_create(const char *file_name)
{
        db_access_t *thiz = NULL;
        thiz = (db_access_t *)calloc(1, sizeof(db_access_t) + sizeof(priv_info));
        if (thiz != NULL) {
            thiz->query     = execute_query;
            thiz->action    = execute_no_query;
            thiz->destroy   = db_access_destroy;
			thiz->free_table= db_free_table;

            priv_info *priv = (priv_info *)thiz->priv;
            strcpy(priv->db_filename, file_name);
            if (sqlite3_open(file_name, &priv->db) < 0) {
                db_access_destroy(thiz);
                thiz = NULL;
            }
        }

        return thiz;
}
