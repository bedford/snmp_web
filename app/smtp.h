#ifndef _SMTP_H_
#define _SMTP_H_

enum SMTP_SECURITY_TYPE
{
	NO_SECURITY,
	USE_TLS,
	USE_SSL,
	DO_NOT_SET
};

typedef struct {
	char receiver_name[32];
	char receiver_mail[32];
} email_receiver_t;

typedef struct {
	char				server_addr[32];
	char				sender_name[32];
	char				sender_email[32];
	char				password[32];
	char				title[64];
	char				content[512];

 	unsigned int        security_type;
	int					port;
	int					email_receiver_cnt;
	email_receiver_t	email_receiver[10];
} email_param_t;

typedef struct _smtp smtp_t;
typedef int (*_send_email)(smtp_t *thiz, email_param_t *param);
typedef void (*_email_destroy)(smtp_t *thiz);

struct _smtp {
	_send_email		send_email;
	_email_destroy	destroy;

    char    		priv[1];
};

smtp_t *smtp_create(void);

#endif
