#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "smtp.h"

#define SOCKET_OK		(0)
#define SOCKET_ERROR	(-1)
#define SOCKET_TIMEOUT	(-2)

#define SSL_OK			(0)
#define SSL_PROBLEM		(-1)

#define INVALID_SOCKET	(-1)

#define TIME_IN_SEC		(3 * 60)
#define MAX_BUF_SIZE	(1024 * 10)

enum ERROR_NUM
{
	RECV_ERROR = 100,
	SEND_ERROR,
	SELECT_ERROR,
	BAD_LOGIN_PASSWORD,
	BAD_DIGEST_RESPONSE,
	BAD_SERVER_NAME,
	LOGIN_NOT_SUPPORTED,
	CONNECTION_CLOSED, // by server
	SERVER_NOT_READY, // remote server
	SERVER_NOT_RESPONDING,
	GETHOSTBY_NAME_ADDR,
	CONNECT_ERROR,

	COMMAND_MAIL_FROM = 300,
	COMMAND_EHLO,
	COMMAND_AUTH_PLAIN,
	COMMAND_AUTH_LOGIN,
	COMMAND_AUTH_CRAMMD5,
	COMMAND_AUTH_DIGESTMD5,
	COMMAND_DIGESTMD5,
    UNDEF_XYZ_RESPONSE,
	COMMAND_DATA,
	COMMAND_QUIT,
	COMMAND_RCPT_TO,
	COMMAND_EHLO_STARTTLS,
	COMMAND_DATABLOCK,
	MSG_BODY_ERROR,
	FILE_NOT_EXIST,
	MSG_TOO_BIG,
	LACK_OF_MEMORY,

	STARTTLS_NOT_SUPPORTED = 400
};

enum SMTP_COMMAND
{
	cmd_INIT,
	cmd_EHLO,
	cmd_AUTH_PLAIN,
	cmd_AUTH_LOGIN,
	cmd_AUTH_CRAMMD5,
	cmd_AUTH_DIGESTMD5,
	cmd_DIGESTMD5,
	cmd_USER,
	cmd_PASSWORD,
	cmd_MAIL_FROM,
	cmd_RCPT_TO,
	cmd_DATA,
	cmd_DATABLOCK,
	cmd_DATAEND,
	cmd_QUIT,
	cmd_STARTTLS
};

typedef struct {
	unsigned int    command;
	int				send_timeout;
	int				recv_timeout;
	int				valid_reply_code;
	unsigned int	error;
} command_entry_t;

typedef struct {
	SSL				*m_ssl;
	SSL_CTX			*m_ctx;

	int             m_socket;
	unsigned int	m_connected;
	char			*recv_buf;
	char			*send_buf;
} priv_info_t;

static command_entry_t command_list[] = {
	{cmd_INIT,		        0,		5*60,	220,	SERVER_NOT_RESPONDING},
	{cmd_EHLO,              5*60,	5*60,	250,	COMMAND_EHLO},
	{cmd_AUTH_PLAIN,        5*60,	5*60,	235,	COMMAND_AUTH_PLAIN},
	{cmd_AUTH_LOGIN,        5*60,	5*60,	334,	COMMAND_AUTH_LOGIN},
	{cmd_AUTH_CRAMMD5,      5*60,	5*60,	334,	COMMAND_AUTH_CRAMMD5},
	{cmd_AUTH_DIGESTMD5,    5*60,	5*60,	334,	COMMAND_AUTH_DIGESTMD5},
	{cmd_DIGESTMD5,         5*60,	5*60,	335,	COMMAND_DIGESTMD5},
	{cmd_USER,              5*60,	5*60,	334,	UNDEF_XYZ_RESPONSE},
	{cmd_PASSWORD,          5*60,	5*60,	235,	BAD_LOGIN_PASSWORD},
	{cmd_MAIL_FROM,         5*60,	5*60,	250,	COMMAND_MAIL_FROM},
	{cmd_RCPT_TO,           5*60,	5*60,	250,	COMMAND_RCPT_TO},
	{cmd_DATA,              5*60,	2*60,	354,	COMMAND_DATA},
	{cmd_DATABLOCK,         3*60,	0,		0,		COMMAND_DATABLOCK},	// Here the valid_reply_code is set to zero because there are no replies when sending data blocks
	{cmd_DATAEND,           3*60,	10*60,	250,	MSG_BODY_ERROR},
	{cmd_QUIT,              5*60,	5*60,	221,	COMMAND_QUIT},
	{cmd_STARTTLS,          5*60,	5*60,	220,	COMMAND_EHLO_STARTTLS}
};

command_entry_t *find_command_entry(unsigned int command)
{
	command_entry_t *entry = NULL;
	int i = 0;
	for (i = 0; i < sizeof(command_list) / sizeof(command_list[0]); ++i) {
		if (command_list[i].command == command) {
			entry = &command_list[i];
			break;
		}
	}

	return entry;
}

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

unsigned char *char_to_unsignedchar(const char *src)
{
	unsigned int length = strlen(src);

	unsigned char *dst = (unsigned char *)malloc(length + 1);
	if (!dst) return NULL;

    int i = 0;
	for (i = 0; i < length; i++) {
		dst[i] = (unsigned char)src[i];
	}

	dst[length] = '\0';

	return dst;
}

// A simple string match
int is_keyword_support(const char* response, const char* keyword)
{
	if (response == NULL || keyword == NULL) {
		return -1;
    }

	int res_len = strlen(response);
	int key_len = strlen(keyword);
	if (res_len < key_len) {
		return -1;
    }

	int pos = 0;
	for (; pos < res_len - key_len + 1; ++pos) {
		if (strncasecmp(keyword, response+pos, key_len) == 0) {
			if (pos > 0 &&
				(response[pos - 1] == '-' ||
				 response[pos - 1] == ' ' ||
				 response[pos - 1] == '=')) {
				if (pos + key_len < res_len) {
					if (response[pos+key_len] == ' ' ||
					   response[pos+key_len] == '=') {
						return 0;
					} else if (pos+key_len+1 < res_len) {
						if (response[pos+key_len] == '\r' &&
						   response[pos+key_len+1] == '\n') {
							return 0;
						}
					}
				}
			}
		}
	}

	return -1;
}

static int socket_check_writeable(int socket_fd, struct timeval timeout)
{
	fd_set fd_write;
	FD_ZERO(&fd_write);
	FD_SET(socket_fd, &fd_write);

	int ret = select(socket_fd + 1, NULL, &fd_write, NULL, &timeout);
	if (ret < 0) {
		return SOCKET_ERROR;
	} else if (ret == 0) {
		return SOCKET_TIMEOUT;
	}

	if (FD_ISSET(socket_fd, &fd_write)) {
		FD_CLR(socket_fd, &fd_write);
		return SOCKET_OK;
	}

	return SOCKET_ERROR;
}

static int send_data_ssl(priv_info_t *priv, command_entry_t *entry)
{
	fd_set fd_write;
	fd_set fd_read;
	int write_blocked_on_read = 0;

    int ret = SSL_OK;

	struct timeval timeout;
	timeout.tv_sec = entry->send_timeout;
	timeout.tv_usec = 0;

	int left = strlen(priv->send_buf);
	int offset = 0;
	while (left > 0) {
		FD_ZERO(&fd_write);
		FD_ZERO(&fd_read);
		FD_SET(priv->m_socket, &fd_write);

		if (write_blocked_on_read) {
			FD_SET(priv->m_socket, &fd_read);
		}

        int status = 0;
		if ((status = select(priv->m_socket + 1, &fd_read, &fd_write, NULL, &timeout)) == SOCKET_ERROR) {
			FD_ZERO(&fd_write);
			FD_ZERO(&fd_read);
			ret = SELECT_ERROR;
			break;
		}

		if (!status) { /* timeout */
			FD_ZERO(&fd_write);
			FD_ZERO(&fd_read);
			ret = SERVER_NOT_RESPONDING;
			break;
		}

		if (FD_ISSET(priv->m_socket, &fd_write) || (write_blocked_on_read && FD_ISSET(priv->m_socket, &fd_read))) {
			write_blocked_on_read = 0;

			/* Try to write */
			int len = SSL_write(priv->m_ssl, priv->send_buf + offset, left);
			switch (SSL_get_error(priv->m_ssl, len)) {
			  /* We wrote something*/
			  case SSL_ERROR_NONE:
				left -= len;
				offset += len;
				break;

				/* We would have blocked */
			  case SSL_ERROR_WANT_WRITE:
				break;

				/* We get a WANT_READ if we're
				   trying to rehandshake and we block on
				   write during the current connection.

				   We need to wait on the socket to be readable
				   but reinitiate our write when it is */
			  case SSL_ERROR_WANT_READ:
				write_blocked_on_read = 1;
				break;

				  /* Some other error */
			  default:
				FD_ZERO(&fd_read);
				FD_ZERO(&fd_write);
				ret = SSL_PROBLEM;
				break;
			}

			if (ret == SSL_PROBLEM) {
				break;
			}
		}
	}

	FD_ZERO(&fd_write);
	FD_ZERO(&fd_read);

	return ret;
}

static int send_data(priv_info_t *priv, command_entry_t *entry)
{
	if (priv->m_ssl != NULL) {
		return send_data_ssl(priv, entry);
	}

	struct timeval timeout;
	timeout.tv_sec = entry->send_timeout;
	timeout.tv_usec = 0;

	int ret = SOCKET_OK;
	int left = strlen(priv->send_buf);
	int idx = 0;
	fd_set fd_write;
	while (left > 0) {
		FD_ZERO(&fd_write);
		FD_SET(priv->m_socket, &fd_write);

        int status = 0;
		if ((status = select(priv->m_socket + 1, NULL, &fd_write, NULL, &timeout)) == SOCKET_ERROR) {
			FD_CLR(priv->m_socket, &fd_write);
			ret = SELECT_ERROR;
			break;
		}

		if (!status) { /* timeout */
			FD_CLR(priv->m_socket, &fd_write);
			ret = SERVER_NOT_RESPONDING;
			break;
		}

		if (status && FD_ISSET(priv->m_socket, &fd_write)) {
			int len = send(priv->m_socket, &priv->send_buf[idx], left, 0);
			if (len == SOCKET_ERROR || len == 0) {
				FD_CLR(priv->m_socket, &fd_write);
				ret = SEND_ERROR;
				break;
			}
			left -= len;
			idx += len;
		}
	}

	FD_CLR(priv->m_socket, &fd_write);

	return ret;
}

static int receive_data_ssl(priv_info_t *priv, command_entry_t *entry)
{
	int ret = 0;
	int offset = 0;
	fd_set fd_read;
	fd_set fd_write;
	int read_blocked_on_write = 0;

	struct timeval timeout;
	timeout.tv_sec = entry->recv_timeout;
	timeout.tv_usec = 0;

	unsigned int finish_flag = 0;
	while(!finish_flag) {
		FD_ZERO(&fd_read);
		FD_ZERO(&fd_write);
		FD_SET(priv->m_socket, &fd_read);

		if (read_blocked_on_write) {
			FD_SET(priv->m_socket, &fd_write);
		}

        int status = 0;
		if ((status = select(priv->m_socket + 1, &fd_read, &fd_write, NULL, &timeout)) == SOCKET_ERROR) {
			FD_ZERO(&fd_read);
			FD_ZERO(&fd_write);
			ret = SELECT_ERROR;
            break;
		}

		if (status == 0) { /* timeout */
			FD_ZERO(&fd_read);
			FD_ZERO(&fd_write);
			ret = SERVER_NOT_RESPONDING;
            break;
		}

		if (FD_ISSET(priv->m_socket, &fd_read)
			|| (read_blocked_on_write && FD_ISSET(priv->m_socket, &fd_write))) {
			while(1) {
				read_blocked_on_write = 0;

				const int buf_len = 1024;
				char buf[buf_len];
				int len = SSL_read(priv->m_ssl, buf, buf_len);

				int ssl_err = SSL_get_error(priv->m_ssl, len);
				if (ssl_err == SSL_ERROR_NONE) {
					if ((offset + len) > (MAX_BUF_SIZE - 1)) {
						FD_ZERO(&fd_read);
						FD_ZERO(&fd_write);
                        ret = LACK_OF_MEMORY;
						break;
					}
					memcpy(priv->recv_buf + offset, buf, len);
					offset += len;
					if (SSL_pending(priv->m_ssl)) {
						continue;
					} else {
						finish_flag = 1;
						break;
					}
				} else if (ssl_err == SSL_ERROR_ZERO_RETURN) {
					finish_flag = 1;
					break;
				} else if (ssl_err == SSL_ERROR_WANT_READ) {
					break;
				} else if (ssl_err == SSL_ERROR_WANT_WRITE) {
					read_blocked_on_write = 1;
					break;
				} else {
					FD_ZERO(&fd_write);
					FD_ZERO(&fd_read);
					ret = SSL_PROBLEM;
                    break;
				}
			}

            if (ret != SSL_OK) {
                break;
            }
		}
	}

	FD_ZERO(&fd_write);
	FD_ZERO(&fd_read);
	priv->recv_buf[offset] = 0;
	if (offset == 0) {
		return CONNECTION_CLOSED;
	}

	return SSL_OK;
}

static int receive_data(priv_info_t *priv, command_entry_t *entry)
{
	if (priv->m_ssl != NULL) {
		return receive_data_ssl(priv, entry);
	}

	int ret = 0;
	fd_set fd_read;

	struct timeval timeout;
	timeout.tv_sec = entry->recv_timeout;
	timeout.tv_usec = 0;

	FD_ZERO(&fd_read);
	FD_SET(priv->m_socket, &fd_read);
	if ((ret = select(priv->m_socket + 1, &fd_read, NULL, NULL, &timeout)) == SOCKET_ERROR) {
		FD_CLR(priv->m_socket, &fd_read);
		return SELECT_ERROR;
	}

	if (ret == 0) {
		FD_CLR(priv->m_socket, &fd_read);
		return SERVER_NOT_RESPONDING;
	}

	if (FD_ISSET(priv->m_socket, &fd_read)) {
		ret = recv(priv->m_socket, priv->recv_buf, MAX_BUF_SIZE, 0);
		if (ret == SOCKET_ERROR) {
			FD_CLR(priv->m_socket, &fd_read);
			return RECV_ERROR;
		}
	}

	FD_CLR(priv->m_socket, &fd_read);
	priv->recv_buf[ret] = 0;
	if (ret == 0) {
		return CONNECTION_CLOSED;
	}

	return SOCKET_OK;
}

static int receive_response(priv_info_t *priv, command_entry_t *entry)
{
	int reply_code = 0;
	unsigned int finish_flag = 0;
	while(!finish_flag) {
		receive_data(priv, entry);

		int len = strlen(priv->recv_buf);
		int offset = 0;
		int begin = 0;
		while(1) {	/* loop for all lines */
			while ((offset + 1) < len) {
				if (priv->recv_buf[offset] == '\r' && priv->recv_buf[offset + 1] == '\n') {
					break;
				}
				++offset;
			}

			if (offset + 1 < len) { /* a new line */
				offset += 2;	/* skip <CRLF> */
				if (offset - begin >= 5) {
					if (isdigit(priv->recv_buf[begin])
                            && isdigit(priv->recv_buf[begin + 1])
                            && isdigit(priv->recv_buf[begin + 2])) {
						/* this is the last line */
						if ((offset - begin == 5) || priv->recv_buf[begin + 3] == ' ') {
							reply_code = (priv->recv_buf[begin] - '0') * 100
                                + (priv->recv_buf[begin + 1] - '0') * 10
                                + priv->recv_buf[begin + 2] - '0';
							finish_flag = 1;
							break;
						}
					}
				}
				begin = offset; /* try to find next line */
			} else {
				break;
			}
		}
	}

	if (reply_code != entry->valid_reply_code) {
		return entry->error;
	}

	return SOCKET_OK;
}

static int say_hello(priv_info_t *priv)
{
	int ret = SOCKET_OK;

	do {
		command_entry_t *entry = find_command_entry(cmd_EHLO);
		snprintf(priv->send_buf, MAX_BUF_SIZE, "EHLO %s\r\n", "domain");
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
			break;
		}

		if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
			priv->m_connected = 1;
			break;
		}
	} while(0);

	return ret;
}

static int init_openssl(priv_info_t *priv)
{
	SSL_library_init();
	SSL_load_error_strings();
	priv->m_ctx = SSL_CTX_new(SSLv23_client_method());
	if (priv->m_ctx == NULL) {
		return SSL_PROBLEM;
	}

	return SSL_OK;
}

static int openssl_connect(priv_info_t *priv)
{
	if (priv->m_ctx == NULL) {
		return SSL_PROBLEM;
	}

	priv->m_ssl = SSL_new(priv->m_ctx);
	if (priv->m_ssl == NULL) {
		return SSL_PROBLEM;
	}

	SSL_set_fd(priv->m_ssl, (int)priv->m_socket);
	SSL_set_mode(priv->m_ssl, SSL_MODE_AUTO_RETRY);

	struct timeval timeout;
	timeout.tv_sec	= TIME_IN_SEC;
	timeout.tv_usec	= 0;

	fd_set fd_write;
	fd_set fd_read;

	int write_blocked = 0;
	int read_blocked = 0;

	int ret = 0;
	while (1) {
		FD_ZERO(&fd_write);
		FD_ZERO(&fd_read);

		if (write_blocked) {
			FD_SET(priv->m_socket, &fd_write);
		}

		if (read_blocked) {
			FD_SET(priv->m_socket, &fd_read);
		}

		if (write_blocked || read_blocked) {
			write_blocked = 0;
			read_blocked = 0;

			if ((ret = select(priv->m_socket + 1, &fd_read, &fd_write, NULL, &timeout)) == SOCKET_ERROR) {
				FD_ZERO(&fd_write);
				FD_ZERO(&fd_read);
				return SELECT_ERROR;
			}

			if (ret == 0) {
				FD_ZERO(&fd_write);
				FD_ZERO(&fd_read);
				return SERVER_NOT_RESPONDING;
			}
		}

		ret = SSL_connect(priv->m_ssl);
		switch (SSL_get_error(priv->m_ssl, ret)) {
			case SSL_ERROR_NONE:
				FD_ZERO(&fd_write);
				FD_ZERO(&fd_read);
				return SSL_OK;
				break;
			case SSL_ERROR_WANT_WRITE:
				write_blocked = 1;
				break;
			case SSL_ERROR_WANT_READ:
				read_blocked = 1;
				break;
			default:
				FD_ZERO(&fd_write);
				FD_ZERO(&fd_read);
				return SSL_PROBLEM;
				break;
		}
	}
}

static int start_tls(priv_info_t *priv)
{
	int ret = SSL_OK;

	do {
		if (is_keyword_support(priv->recv_buf, "STARTTLS")) {
			ret = STARTTLS_NOT_SUPPORTED;
			break;
		}

		command_entry_t *entry = find_command_entry(cmd_STARTTLS);
		snprintf(priv->send_buf, MAX_BUF_SIZE, "STARTTLS\r\n");
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
			break;
		}

		if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
			break;
		}

		ret = openssl_connect(priv);
	} while(0);

	return ret;
}

static int connect_server_login_method(priv_info_t *priv, email_param_t *param)
{
	int ret = SOCKET_OK;

	do {
		command_entry_t *entry = find_command_entry(cmd_AUTH_LOGIN);
		snprintf(priv->send_buf, MAX_BUF_SIZE, "AUTH LOGIN\r\n");
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
			break;
		}

		if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
			break;
		}

		/* send login user */
		char encode_buffer[128] = {0};
		base64_encode((unsigned char *)param->sender_email, strlen(param->sender_email),
                encode_buffer, sizeof(encode_buffer)); /* base64 */

		entry = find_command_entry(cmd_USER);
		snprintf(priv->send_buf, MAX_BUF_SIZE, "%s\r\n", encode_buffer);
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
			break;
		}

		if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
			break;
		}

		/* send password */
		memset(encode_buffer, 0, sizeof(encode_buffer));
		base64_encode((unsigned char *)param->password, strlen(param->password),
                encode_buffer, sizeof(encode_buffer)); /* base64 */
		entry = find_command_entry(cmd_PASSWORD);
		snprintf(priv->send_buf, MAX_BUF_SIZE, "%s\r\n", encode_buffer);
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
			break;
		}

		if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
			break;
		}
	} while(0);

 	return ret;
}

static int connect_server_plain_method(priv_info_t *priv, email_param_t *param)
{
	int ret = SOCKET_OK;

	do {
		command_entry_t *entry = find_command_entry(cmd_AUTH_PLAIN);
		snprintf(priv->send_buf, MAX_BUF_SIZE, "%s^%s^%s",
                param->sender_email, param->sender_email, param->password);
		unsigned int length = strlen(priv->send_buf);
		unsigned char *ustr_login = char_to_unsignedchar(priv->send_buf);
		unsigned int i = 0;
		for(i = 0; i < length; i++) {
			if (ustr_login[i] == 94) {
				ustr_login[i] = 0;
			}
		}

		char encode_buffer[128] = {0};
		base64_encode(ustr_login, strlen((char *)ustr_login), encode_buffer, sizeof(encode_buffer)); /* base64 */
		free(ustr_login);
		ustr_login = NULL;
		snprintf(priv->send_buf, MAX_BUF_SIZE, "AUTH PLAIN %s\r\n", encode_buffer);
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
			break;
		}

		if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
			break;
		}
	} while(0);

	return ret;
}

static void say_quit(priv_info_t *priv)
{
	command_entry_t *entry = find_command_entry(cmd_QUIT);
	/* QUIT <CRLF> */
	snprintf(priv->send_buf, MAX_BUF_SIZE, "QUIT\r\n");
	priv->m_connected = 0;

	send_data(priv, entry);
	receive_response(priv, entry);
	entry = NULL;
}

static void disconnect_server(priv_info_t *priv)
{
	if (priv->m_connected) {
		say_quit(priv);
	}

	if (priv->m_socket) {
		close(priv->m_socket);
	}

	priv->m_socket = INVALID_SOCKET;
}

static void cleanup_openssl(priv_info_t *priv)
{
	if (priv->m_ssl != NULL) {
		SSL_shutdown(priv->m_ssl);
		SSL_free(priv->m_ssl);
		priv->m_ssl = NULL;
	}

	if (priv->m_ctx != NULL) {
		SSL_CTX_free(priv->m_ctx);
		priv->m_ctx = NULL;
		ERR_remove_state(0);
		ERR_free_strings();
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
	}
}

static int connect_server(priv_info_t *priv, email_param_t *param)
{
	int ret = SOCKET_OK;
	do {
		struct timeval timeout;
		timeout.tv_sec	= TIME_IN_SEC;
		timeout.tv_usec = 0;

		int m_socket = socket(PF_INET, SOCK_STREAM, 0);
		if (m_socket == INVALID_SOCKET) {
			ret = INVALID_SOCKET;
			break;
		}

		struct sockaddr_in sock_addr;
		sock_addr.sin_family	= AF_INET;
		sock_addr.sin_port		= htons(param->port);
		if ((sock_addr.sin_addr.s_addr = inet_addr(param->server_addr)) == INADDR_NONE) {
			struct hostent *host = gethostbyname(param->server_addr);
			if (host) {
				memcpy(&sock_addr.sin_addr, host->h_addr_list[0], host->h_length);
			} else {
				close(m_socket);
				ret = GETHOSTBY_NAME_ADDR;
				break;
			}
		}

		//start non-blocking mode for socket
		int flags = fcntl(m_socket, F_GETFL);
		flags |= O_NONBLOCK;
		fcntl(m_socket, F_SETFL, flags);

		if (connect(m_socket, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == SOCKET_ERROR) {
			if (errno != EINPROGRESS) {
				close(m_socket);
				ret = CONNECT_ERROR;
				break;
			}
		} else {
			break;
		}

		if ((ret = socket_check_writeable(m_socket, timeout)) != SOCKET_OK) {
			break;
		}

		priv->m_socket = m_socket;
		if ((param->security_type == USE_TLS) || (param->security_type == USE_SSL)) {
			if ((ret = init_openssl(priv)) != SSL_OK) {
				break;
			}

			if (param->security_type == USE_SSL) {
				if ((ret = openssl_connect(priv)) != SSL_OK) {
					break;
				}
			}
		}

		command_entry_t *entry = find_command_entry(cmd_INIT);
		if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
			break;
		}

		if ((ret = say_hello(priv)) != SOCKET_OK) {
			break;
		}


		if (param->security_type == USE_TLS) {
			if ((ret = start_tls(priv)) != SSL_OK) {
				break;
			}

			if ((ret = say_hello(priv)) != SOCKET_OK) {
				break;
			}
		}

		if (is_keyword_support(priv->recv_buf, "AUTH") == 0) {
			if (is_keyword_support(priv->recv_buf, "LOGIN") == 0) {
				ret = connect_server_login_method(priv, param);
			} else if (is_keyword_support(priv->recv_buf, "PLAIN") == 0) {
				ret = connect_server_plain_method(priv, param);
			} else {
				ret = LOGIN_NOT_SUPPORTED;
			}
		}
	} while(0);

	if (ret != SOCKET_OK) {
		if (priv->recv_buf[0] == '5' && priv->recv_buf[1] == '3' && priv->recv_buf[2] == '0') {
			priv->m_connected = 0;
			disconnect_server(priv);
		}
	}

	return ret;
}

static void format_header(priv_info_t *priv, email_param_t *param)
{
	char month[][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
					   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	// date/time check
	time_t rawtime;
	time(&rawtime);
	struct tm *timeinfo = localtime(&rawtime);

	char *header = priv->send_buf;
	// Date: <space> <dd> <space> <mon> <space> <yy> <space> <hh> ":" <mm> ":" <ss> <space> <zone> <CRLF>
	snprintf(header, MAX_BUF_SIZE, "Date: %d %s %d %d:%d:%d\r\n",
			 timeinfo->tm_mday, month[timeinfo->tm_mon], timeinfo->tm_year + 1900,
			 timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	// From: <SP> <sender>  <SP> "<" <sender-email> ">" <CRLF>
	strcat(header, "From: ");
	strcat(header, param->sender_name);

	strcat(header, " <");
	strcat(header, param->sender_email);
	strcat(header, ">\r\n");

	// To: <SP> <remote-user-mail> <CRLF>
	int i = 0;
	strcat(header, "To: ");
	for (i = 0; i < param->email_receiver_cnt; i++) {
		if (i > 0) {
			strcat(header, ",");
		}
		strcat(header, param->email_receiver[i].receiver_name);
		strcat(header, "<");
		strcat(header, param->email_receiver[i].receiver_mail);
		strcat(header, ">");
	}
	strcat(header, "\r\n");

	// Subject: <SP> <subject-text> <CRLF>
	strcat(header, "Subject: ");
	strcat(header, param->title);
	strcat(header, "\r\n");

	// MIME-Version: <SP> 1.0 <CRLF>
	strcat(header,"MIME-Version: 1.0\r\n");
	// no attachments
	strcat(header, "Content-type: text/plain; charset=\"UTF-8\"\r\n");
	strcat(header, "Content-Transfer-Encoding: 7bit\r\n\r\n");

	// done
}

static int send_email(smtp_t *thiz, email_param_t *param)
{
	priv_info_t *priv = (priv_info_t *)thiz->priv;

	int ret = SOCKET_OK;
	do {
		if (priv->m_socket == INVALID_SOCKET) {
			if ((ret = connect_server(priv, param)) != SOCKET_OK) {
				break;
			}
		}

		/* MAIL <space> FROM:<sender><CRLF>*/
		command_entry_t *entry = NULL;
		entry = find_command_entry(cmd_MAIL_FROM);
		snprintf(priv->send_buf, MAX_BUF_SIZE, "MAIL FROM:<%s>\r\n", param->sender_email);
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
				break;
			break;
		}

		if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
			break;
		}

		/* RCPT <space> TO:<receivers><CRLF>*/
		int i = 0;
		entry = find_command_entry(cmd_RCPT_TO);
		for (i = 0; i < param->email_receiver_cnt; i++) {
			snprintf(priv->send_buf, MAX_BUF_SIZE, "RCPT TO:<%s>\r\n",
				param->email_receiver[i].receiver_mail);
			if ((ret = send_data(priv, entry)) != SOCKET_OK) {
				break;
			}

			if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
				break;
			}
		}
		if (ret != SOCKET_OK) {
			break;
		}

		/* DATA <CRLF>*/
		entry = find_command_entry(cmd_DATA);
		snprintf(priv->send_buf, MAX_BUF_SIZE, "DATA\r\n");
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
			break;
		}

		if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
			break;
		}

		/* send header */
		entry = find_command_entry(cmd_DATABLOCK);
		format_header(priv, param);
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
			break;
		}

		snprintf(priv->send_buf, MAX_BUF_SIZE, "%s\r\n", param->content);
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
			break;
		}

		/* <CRLF>.<CRLF>*/
		entry = find_command_entry(cmd_DATAEND);
		snprintf(priv->send_buf, MAX_BUF_SIZE, "\r\n.\r\n");
		if ((ret = send_data(priv, entry)) != SOCKET_OK) {
			break;
		}

		if ((ret = receive_response(priv, entry)) != SOCKET_OK) {
			break;
		}
	} while(0);

	if (priv->m_connected) {
		disconnect_server(priv);
	}
	cleanup_openssl(priv);

    return ret;
}

void smtp_destroy(smtp_t *thiz)
{
	if (thiz != NULL) {
		priv_info_t *priv = (priv_info_t *)thiz->priv;
		if (priv->send_buf != NULL) {
			free(priv->send_buf);
			priv->send_buf = NULL;
		}

		if (priv->recv_buf != NULL) {
			free(priv->recv_buf);
			priv->recv_buf = NULL;
		}

		memset(thiz, 0, sizeof(smtp_t) + sizeof(priv_info_t));
		free(thiz);
		thiz = NULL;
	}
}

smtp_t *smtp_create(void)
{
    smtp_t *thiz = (smtp_t *)calloc(1, sizeof(smtp_t) + sizeof(priv_info_t));
    if (thiz != NULL) {
		thiz->send_email 	= send_email;
		thiz->destroy		= smtp_destroy;

		priv_info_t *priv = (priv_info_t *)thiz->priv;
		priv->send_buf = (char *)calloc(1, MAX_BUF_SIZE);
		priv->recv_buf = (char *)calloc(1, MAX_BUF_SIZE);
		priv->m_socket = INVALID_SOCKET;
		priv->m_connected = 0;
    }

    return thiz;
}
