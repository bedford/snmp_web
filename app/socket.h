#ifndef _SOCKET_H_
#define _SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SOCKET_OK       (0)
#define SOCKET_FAIL     (-1)
#define SOCKET_TIMEOUT  (-2)
#define SOCKET_SHUTDOWN (-3)

int socket_create_tcp(void);

void socket_set_nonblock(int socket_fd);

void socket_set_block(int socket_fd);


/**
 * @brief       connect to the dest_addr host, dest_port 
 *
 * @param       socket_fd [I ] client socket file description
 * @param       dest_addr [I ] destination host address
 * @param       dest_port [I ] destination port
 *
 * @return      result of connecting
 * @retval      -1      fail
 * @retval      0       success
 */

int socket_connect(int socket_fd, char *dest_addr, int dest_port);


/**
 * @brief       socket_check_writable To check whether the socket is writable
 *
 * @param       socket_fd [I ] The socket file description to check
 * @param       ms      [I ]   timeout up bound (Unit: ms)
 *
 * @return      result of able to write or not
 * @retval      -2      timeout error
 * @retval      -1      unwritable
 * @retval      0       writable
 */

int socket_write(int socket_fd, char *buf, int length, int ms);


/**
 * @brief       socket_check_readable To check whether the socket is readable
 *
 * @param       socket_fd [I ] The socket file description to check
 * @param       ms      [I ]   timeout up bound (Unit: ms)
 *
 * @return      result of able to read or not
 * @retval      -3      server shutdown
 * @retval      -2      timeout error
 * @retval      -1      unreadable
 * @retval      0       readable
 */

int socket_read_length(int socket_fd, char *buf, int max_length, int ms);
int socket_read(int socket_fd, char *buf, int req_length, int ms);

/**
 * @brief       socket_clear_recv_buffer To clear socket receive buffer
 *
 * @param       socket_fd [I ] The socket file description to clear
 */

void socket_clear_recv_buffer(int socket_fd);

int socket_bind(int socket_fd, int port);

int socket_listen(int socket_fd, int max_request);

int socket_accept(int socket_fd, int ms, int *client_fd);

int socket_close(int socket_fd);

int socket_multicast_write(char *multicast_ip, int multicast_port,
                           int local_port, char *buffer, int len);

int socket_multicast_server_init(char *multicast_ip, int multicast_port);

int socket_multicast_read(int multicast_fd, int multicast_port,
                        char *buf, int req_length, char *remote_addr);

int socket_read_udp(int socket_fd, char *buf, int req_length, char *remote_addr);

int socket_udpserver_create(int port);

int socket_udp_broadcast(int broadcast_port, char *buffer, int len);

int socket_udp_send_packet(char *ip, int port, char *buffer, int len);

void socket_set_nodelay(int socket_fd);

#ifdef __cplusplus
}
#endif

#endif
