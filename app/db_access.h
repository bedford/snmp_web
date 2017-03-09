#ifndef _DB_ACCESS_H_
#define _DB_ACCESS_H_

typedef struct {
	char	**result;
	int		row;
	int		column;
	char	*error_msg;
} query_result_t;

struct _db_access;
typedef struct _db_access db_access_t;

typedef int (*_db_access_action)(db_access_t *thiz, const char *sql, char *err_msg);
typedef int (*_db_access_query)(db_access_t *thiz, const char *sql, query_result_t *query_result);
typedef int (*_db_access_free_table)(db_access_t *thiz, char **result);
typedef void (*_db_access_destroy)(db_access_t *thiz);

struct _db_access
{
    _db_access_query    	query;
    _db_access_action   	action;
    _db_access_destroy  	destroy;
	_db_access_free_table	free_table;

    char priv[1];
};

db_access_t *db_access_create(const char *file_name);

#endif
