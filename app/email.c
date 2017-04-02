#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "socket.h"
#include "email.h"

typedef struct {
	int 	client_fd;
	char	buf[512];
	int		port;
} priv_info_t;

static char base64_table[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_encode(unsigned char* pBase64, int nLen, char* pOutBuf, int nBufSize)
{
	int i = 0;
	int j = 0;
	int nOutStrLen = 0;

	/* nOutStrLen does not contain null terminator. */
	nOutStrLen = nLen / 3 * 4 + (0 == (nLen % 3) ? 0 : 4);
	if ( pOutBuf && nOutStrLen < nBufSize )
	{
		char cTmp = 0;
		for ( i = 0, j = 0; i < nLen; i += 3, j += 4 )
		{
			/* the first character: from the first byte. */
			pOutBuf[j] = base64_table[pBase64[i] >> 2];

			/* the second character: from the first & second byte. */
			cTmp = (char)((pBase64[i] & 0x3) << 4);
			if ( i + 1 < nLen )
			{
				cTmp |= ((pBase64[i + 1] & 0xf0) >> 4);
			}
			pOutBuf[j+1] = base64_table[(int)cTmp];

			/* the third character: from the second & third byte. */
			cTmp = '=';
			if ( i + 1 < nLen )
			{
				cTmp = (char)((pBase64[i + 1] & 0xf) << 2);
				if ( i + 2 < nLen )
				{
					cTmp |= (pBase64[i + 2] >> 6);
				}
				cTmp = base64_table[(int)cTmp];
			}
			pOutBuf[j + 2] = cTmp;

			/* the fourth character: from the third byte. */
			cTmp = '=';
			if ( i + 2 < nLen )
			{
				cTmp = base64_table[pBase64[i + 2] & 0x3f];
			}
			pOutBuf[j + 3] = cTmp;
		}

		pOutBuf[j] = '\0';
	}

	return nOutStrLen + 1;
}

static int get_response(priv_info_t *priv)
{
	memset(priv->buf, 0, sizeof(priv->buf));
	int len = socket_read_length(priv->client_fd, priv->buf, sizeof(priv->buf), 5000);
	if (len <= 0) {
		return -1;
	} else {
		printf("%s\n", priv->buf);
	}

	socket_clear_recv_buffer(priv->client_fd);

	return 0;
}

static int create_connect(priv_info_t *priv, char *server_domain)
{
	priv->client_fd = socket_create_tcp();
	socket_set_nonblock(priv->client_fd);

	if (socket_connect(priv->client_fd, server_domain, priv->port)) {
		return -1;
	}

	return get_response(priv);
}

static int login(priv_info_t *priv, char *user, char *password)
{
	char cmd[128] = {0};
	sprintf(cmd, "EHLO %s\r\n", user);
	int ret = socket_write(priv->client_fd, cmd, strlen(cmd), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	if (get_response(priv)) {
		return -1;
	}

	strcpy(cmd, "AUTH LOGIN\r\n");
	ret = socket_write(priv->client_fd, cmd, strlen(cmd), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	if (get_response(priv)) {
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	base64_encode((unsigned char *)user, strlen(user), cmd, sizeof(cmd)); /* base64 */
	strcat(cmd, "\r\n");
	ret = socket_write(priv->client_fd, cmd, strlen(cmd), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	if (get_response(priv)) {
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	base64_encode((unsigned char *)password, strlen(password), cmd, sizeof(cmd)); /* base64 */
	strcat(cmd, "\r\n");
	ret = socket_write(priv->client_fd, cmd, strlen(cmd), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	if (get_response(priv)) {
		return -1;
	}

	char response_code[4] = {0};
	strncpy(response_code, priv->buf, 3);
	int code = atoi(response_code);
	if (code != 235) {
		return -1;
	}

	return 0;
}

static int send_mail_head(priv_info_t *priv, email_param_t *param)
{
	char cmd[256] = {0};

	sprintf(cmd, "MAIL FROM: <%s>\r\n", param->user);
	int ret = socket_write(priv->client_fd, cmd, strlen(cmd), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	if (get_response(priv)) {
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "RCPT TO: <%s>\r\n", param->to_addr);
	ret = socket_write(priv->client_fd, cmd, strlen(cmd), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	if (get_response(priv)) {
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	strcpy(cmd, "DATA\r\n");
	ret = socket_write(priv->client_fd, cmd, strlen(cmd), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	if (get_response(priv)) {
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "From: %s\r\n"
			"TO: %s\r\n"
			"Subject: %s\r\n"
			"MIME-Version: 1.0\r\n\r\n",
			param->user, param->to_addr, param->title);
	ret = socket_write(priv->client_fd, cmd, strlen(cmd), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	return 0;
}

static int send_text(priv_info_t *priv, email_param_t *param)
{
	int ret = socket_write(priv->client_fd, param->content, strlen(param->content), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	char cmd[128] = {0};
	strcat(cmd, "\r\n.\r\n");
	ret = socket_write(priv->client_fd, cmd, strlen(cmd), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	if (get_response(priv)) {
		return -1;
	}

	char response_code[4] = {0};
	strncpy(response_code, priv->buf, 3);
	int code = atoi(response_code);

	memset(cmd, 0, sizeof(cmd));
	strcpy(cmd, "QUIT\r\n");
	ret = socket_write(priv->client_fd, cmd, strlen(cmd), 5000);
	if (ret != SOCKET_OK) {
		return -1;
	}

	get_response(priv);
	if (code != 250) {
		return -1;
	}

	return 0;
}

static int send_email(email_t *thiz, email_param_t *param)
{
	priv_info_t *priv = (priv_info_t *)thiz->priv;
	int ret = 0;
	do {
		if (create_connect(priv, param->smtp_server)) {
			ret = -1;
			break;
		}

		if (login(priv, param->user, param->password)) {
			ret = -1;
			break;
		}

		if (send_mail_head(priv, param)) {
			ret = -1;
			break;
		}

		if (send_text(priv, param)) {
			ret = -1;
			break;
		}
	} while(0);

	socket_close(priv->client_fd);
	priv->client_fd = -1;

	return ret;
}

static void email_destroy(email_t *thiz)
{
    if (thiz != NULL) {
        priv_info_t *priv = (priv_info_t *)thiz->priv;
		if (priv->client_fd) {
			socket_close(priv->client_fd);
			priv->client_fd = -1;
		}

        memset(thiz, 0, sizeof(email_t) + sizeof(priv_info_t));
        free(thiz);
        thiz = NULL;
    }
}

email_t *email_create(void)
{
    email_t *thiz = calloc(1, sizeof(email_t) + sizeof(priv_info_t));
    if (thiz != NULL) {
        thiz->send_email	= send_email;
        thiz->destroy		= email_destroy;

        priv_info_t *priv = (priv_info_t *)thiz->priv;
		priv->client_fd = -1;
		priv->port = 25;
		memset(priv->buf, 0, sizeof(priv->buf));
    }

    return thiz;
}
