#ifndef _DEBUG_H_
#define _DEBUG_H_

void print_buf(unsigned char *buf, int len);

void print_com_info(int com_index, char *device_name, int dir, char *buf, int len, int err_code);

#endif
