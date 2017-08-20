#ifndef _DB_ACCESS_H_
#define _DB_ACCESS_H_

/**
 * @brief   数据库查询操作返回结果
 */
typedef struct {
	char	**result;
	int		row;
	int		column;
	char	*error_msg;
} query_result_t;

struct _db_access;
typedef struct _db_access db_access_t;

/**
 * @brief   数据库非查询操作接口声明
 */
typedef int (*_db_access_action)(db_access_t *thiz, const char *sql, char *err_msg);

/**
 * @brief   数据库查询操作接口声明
 */
typedef int (*_db_access_query)(db_access_t *thiz, const char *sql, query_result_t *query_result);

/**
 * @brief   数据库查询返回资源释放接口声明
 */
typedef int (*_db_access_free_table)(db_access_t *thiz, char **result);

/**
 * @brief   数据库操作对象销毁
 */
typedef void (*_db_access_destroy)(db_access_t *thiz);

struct _db_access
{
    _db_access_query    	query;
    _db_access_action   	action;
    _db_access_destroy  	destroy;
	_db_access_free_table	free_table;

    char priv[1];
};

/**
 * @brief   db_access_create    创建数据库操作对象
 * @param   file_name           数据库文件名称
 * @return
 */
db_access_t *db_access_create(const char *file_name);

#endif
