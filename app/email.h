#ifndef _EMAIL_H_
#define _EMAIL_H_

typedef struct {
	char	smtp_server[32];
	char	to_addr[32];
	char	user[32];
	char	password[32];
	char	title[64];
	char	content[512];
} email_param_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _email email_t;

typedef int (*_send_email)(email_t *thiz, email_param_t *param);
typedef void (*_email_destroy)(email_t *thiz);

struct _email
{
    _send_email		send_email;
    _email_destroy	destroy;

    char			priv[1];
};

email_t *email_create(void);

#ifdef __cplusplus
}
#endif

#endif
