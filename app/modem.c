#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "uart.h"
#include "modem.h"

/**
 * @brief   AT命令枚举
 */
enum
{
    AT = 0,
    AT_CSQ,
    AT_CREG,
    AT_CMGF,
    AT_CMGS,
    AT_CSCA,
};

/**
 * @brief   私有成员
 */
typedef struct
{
    uart_t  *uart;          /* 串口句柄指针 */
    char    sca[16];        /* 短信中心 */
    char    sca_pdu[16];    /* 短信中心PDU码 */
} priv_info_t;

#define AT_CMD_TAILER_LEN   (1)

/**
 * @brief   utf8_to_unicode utf8转成unicode
 * @param   utf8            utf8格式字符串地址
 * @param   unicode         返回的unicode值
 * @return                  该字符占的字节数，以便对整个字符串进行移位处理
 */
static int utf8_to_unicode(const char *utf8, unsigned int *unicode)
{
    int ret = 0;
    *unicode = 0;
    if ((*utf8 & 0x80) == 0x00) {
        *unicode = *utf8;
        ret = 1;
    } else if ((*utf8 & 0xE0) == 0xC0) {
        *unicode += (utf8[0] & 0x1F) << 6;
        *unicode += (utf8[1] & 0x3F);
        ret = 2;
    } else if ((*utf8 & 0xF0) == 0xE0) {
        *unicode += (utf8[0] & 0x0F) << 12;
        *unicode += (utf8[1] & 0x3F) << 6;
        *unicode += (utf8[2] & 0x3F);
        ret = 3;
    } else {
        ret = -1;
    }

    return ret;
}

/**
 * @brief   utf8_string_convert utf8字符串转换成unicode编码 
 * @param   utf8_string         源utf8字符串指针
 * @param   dst                 目标编码指针
 */
static void utf8_string_convert(char *utf8_string, char *dst)
{
    char *p = utf8_string;
    char buf[1024] = {0};
    int step = 0;
    unsigned int unicode = 0;
    char tmp[8] = {0};
    while (*p != '\0') {
        step = utf8_to_unicode(p, &unicode);
        if (step < 0) {
            break;
        } else {
            sprintf(tmp, "%04x", unicode);
            strncat(buf, tmp, strlen(tmp));
        }

        p += step;
    }

    strcpy(dst, buf);
}

/**
 * @brief   send_tailer 发送AT指令结束符0x0D
 * @param   uart        串口操作句柄
 */
static inline void send_tailer(uart_t *uart)
{
    char tailer[AT_CMD_TAILER_LEN] = {0x0D};
    uart->write(uart, tailer, AT_CMD_TAILER_LEN, 1);
}

/**
 * @brief   send_ascii  发送ASCII码 
 * @param   uart        串口操作句柄
 * @param   buf         发送缓存指针
 */
static inline void send_ascii(uart_t *uart, const char *buf)
{
    int len = strlen(buf);
    uart->write(uart, buf, len, 3);
}

/**
 * @brief   at_cmd_implement    AT指针返回数据处理 
 * @param   priv                私有成员变量
 * @param   cmd                 发送的AT指令
 * @param   cmd_index           发送的AT指令检举编号
 * @return
 */
static int at_cmd_implement(priv_info_t *priv, const char *cmd, unsigned int cmd_index)
{
    uart_t *uart = priv->uart;
    send_ascii(uart, cmd);
    send_tailer(uart);

    char tmp[256] = {0};
    int ret = uart->read(uart, tmp, 256, 2);
    if (ret > 0) {
        printf("tmp %s\n", tmp);
    }

    int i = 0;
    ret = -1;
    switch (cmd_index) {
    case AT_CSCA:   /* 查询短信中心号码 */
		if (strstr(tmp, "OK") != NULL) {
	        if (strstr(tmp, cmd) != NULL) {
	            i = 0;
	            char *p = strchr(tmp, '\"');
	            while (*(++p) != '\"') {
	                if (*p != '+') {
	                    priv->sca[i++] = *p;
	                }
	            }
	            priv->sca[i] = '\0';
	            ret = 0;
	        }
		}
        break;
    case AT_CMGS:
        if ((strstr(tmp, "OK") != NULL) || (strstr(tmp, ">") != NULL)) {
            ret = 0;
        }
        break;
    case AT_CSQ:
    case AT_CREG:
    case AT_CMGF:
    case AT:
    default:
        if (strstr(tmp, "OK") != NULL) {
            ret = 0;
        }
        break;
    }

    return ret;
}

/**
 * @brief   phone_to_pdu    电话号码转换成PDU编码格式 
 * @param   src             电话号码字符串
 * @param   dst             PDU编码格式目标串
 */
static void phone_to_pdu(char *src, char *dst)
{
    char tmp[16] = {0};
    if (src[0] != '8') {    /* 如果电话号码前不含86，则添加 */
        sprintf(tmp, "86%s", src);
    } else {
        strcpy(tmp, src);
    }

    int len = strlen(tmp) + 1;
    if ((len % 2) == 0) {   /* 加上前缀后的号码长度是否为奇数，如果是，则最后补F */
        strncat(tmp, "F", 1);
    }

    int i = 0;  /* 奇偶位对调 */
    for (i = 0; i < len; i += 2) {
        dst[i] = tmp[i+1];
        dst[i+1] = tmp[i];
    }
}

/**
 * @brief   modem_connected 通过发送AT指令,确认SMS模块是否正常 
 * @param   thiz            SMS模块对象指针
 * @return
 */
static int modem_connected(modem_t *thiz)
{
    priv_info_t *priv = (priv_info_t *)thiz->priv;

    int ret = -1;
    if (at_cmd_implement(priv, "AT", AT) == 0) {
        ret = 0;
    }

    return ret;
}

/**
 * @brief   modem_get_sca   获取短信中心号码
 * @param   thiz
 * @param   sca
 * @return
 */
static int modem_get_sca(modem_t *thiz, char *sca)
{
    priv_info_t *priv = (priv_info_t *)thiz->priv;
    int ret = -1;
    if (at_cmd_implement(priv, "AT+CSCA?", AT_CSCA) == 0) {
        /* 记录 短信中心号码 */
        strcpy(sca, priv->sca);
        phone_to_pdu(priv->sca, priv->sca_pdu);
        ret = 0;
    }

    return ret;
}

/**
 * @brief   modem_set_mode  设置短信发送模式 
 * @param   thiz
 * @param   sms_mode
 * @return
 */
static int modem_set_mode(modem_t *thiz, sms_mode_t sms_mode)
{
    priv_info_t *priv = (priv_info_t *)thiz->priv;

    int ret = -1;
    char cmd[16] = {0};
    sprintf(cmd, "AT+CMGF=%d", sms_mode);
    if (at_cmd_implement(priv, cmd, AT_CMGF) == 0) {
        ret = 0;
    }

    return ret;
}

/**
 * @brief   append_packet   读取短信发送返回结果 
 * @param   priv
 * @param   sms_packet
 * @return
 */
static int append_packet(priv_info_t *priv, char *sms_packet)
{
    uart_t *uart = priv->uart;
    send_ascii(uart, sms_packet);
    send_tailer(uart);

    char tmp[512] = {0};
    int ret = uart->read(uart, tmp, 512, 5);
    if (ret > 0) {
        printf("tmp %s\n", tmp);
        if (strstr(tmp, "OK") != NULL) {
            return 0;
        }
    }

    return -1;
}

/**
 * @brief   send_sms    发送短信 
 * @param   priv        私有成员变量
 * @param   phone_num   接收人手机号码
 * @param   content     短信内容
 * @return
 */
static int send_sms(priv_info_t *priv, char *phone_num, char *content)
{
    static unsigned char pdu_flag = 0;

    char pdu_string[1024] = {0};
    char sms_packet[512] = {0};
    char phone_num_pdu[32] = {0};

    if (strlen(content) == 0) {
        pdu_flag = 0;
        return 0;
    }

    if (pdu_flag == 0) {
        pdu_flag = 1;
        utf8_string_convert(content, pdu_string);
        phone_to_pdu(phone_num, phone_num_pdu);
    } else {
        strcpy(pdu_string, content);
        strcpy(phone_num_pdu, phone_num);
    }

    int sms_text_len = strlen(pdu_string);
    sms_text_len = (sms_text_len > 280) ? 280 : sms_text_len;
    char tmp[288] = {0};
    strncpy(tmp, pdu_string, sms_text_len);
    tmp[sms_text_len] = '\0';

    int len = (strlen(priv->sca_pdu) + 2) / 2;
    sprintf(sms_packet, "%02x", len);
    strncat(sms_packet, "91", 2);
    strncat(sms_packet, priv->sca_pdu, strlen(priv->sca_pdu));

    int header_len = strlen(sms_packet);

    strncat(sms_packet, "1100", 4);
    len = strlen(phone_num_pdu) - 1;    /* 不计算'F'和'+'字符 */

	char len_buf[8] = {0};
	sprintf(len_buf, "%02x", len);
	strncat(sms_packet, len_buf, 2);
    strncat(sms_packet, "91", 2);

    strncat(sms_packet, phone_num_pdu, strlen(phone_num_pdu));
	strncat(sms_packet, "000800", 6);

    len = sms_text_len / 2;
	sprintf(len_buf, "%02x", len);
	strncat(sms_packet, len_buf, 2);

    strncat(sms_packet, tmp, strlen(tmp));
	sprintf(len_buf, "%c", 0x1A);
	strncat(sms_packet, len_buf, 1);

    len = strlen(sms_packet + header_len) / 2;

    char cmd[16] = {0};
    sprintf(cmd, "AT+CMGS=%d", len);
    if (at_cmd_implement(priv, cmd, AT_CMGS) != 0) {
        pdu_flag = 0;
        return -1;
    }

    if (append_packet(priv, sms_packet) != 0) {
        pdu_flag = 0;
        return -1;
    }

    if (strlen(pdu_string) > 280) {
        send_sms(priv, phone_num, (char *)(content + 280));
    }

    pdu_flag = 0;

    return 0;
}

/**
 * @brief   modem_send_sms  发送短信外部调用接口 
 * @param   thiz
 * @param   phone_num
 * @param   content
 * @return
 */
static int modem_send_sms(modem_t *thiz, char *phone_num, char *content)
{
    priv_info_t *priv = (priv_info_t *)thiz->priv;
    return send_sms(priv, phone_num, content);
}

/**
 * @brief   modem_destroy   销毁短信发送对象 
 * @param   thiz
 */
static void modem_destroy(modem_t *thiz)
{
    if (thiz != NULL) {
        priv_info_t *priv = (priv_info_t *)thiz->priv;
        uart_t *uart = priv->uart;
        if (uart) {
            uart->destroy(uart);
            uart = NULL;
        }
        memset(thiz, 0, sizeof(modem_t) + sizeof(priv_info_t));
        free(thiz);
        thiz = NULL;
    }
}

/**
 * @brief   modem_create    创建短信发送对象 
 * @return
 */
modem_t *modem_create(void)
{
    modem_t *thiz = calloc(1, sizeof(modem_t) + sizeof(priv_info_t));
    if (thiz != NULL) {
        thiz->connected = modem_connected;
        thiz->set_mode  = modem_set_mode;
        thiz->send_sms  = modem_send_sms;
        thiz->get_sca   = modem_get_sca;
        thiz->destroy   = modem_destroy;

        priv_info_t *priv = (priv_info_t *)thiz->priv;

        uart_param_t uart_param;
        uart_param.device_index = 1;
        uart_param.baud = UART_BAUD_9600;
        uart_param.bits = UART_BITS_8;
        uart_param.parity = UART_PARITY_NONE;
        uart_param.stops = UART_STOP_1;
        uart_t *uart = uart_create(&uart_param);
        if (uart) {
            uart->open(uart);
            priv->uart = uart;
        }
    }

    return thiz;
}
