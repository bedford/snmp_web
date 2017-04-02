#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>

#include "socket.h"

#define DEFAULT_TIMEOUT         3000

int socket_create_tcp(void)
{
        int socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        return socket_fd;
}

void socket_set_nonblock(int socket_fd)
{
        int flags = fcntl(socket_fd, F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(socket_fd, F_SETFL, flags);
}

void socket_set_block(int socket_fd)
{
        int flags = fcntl(socket_fd, F_GETFL);
        flags &= ~O_NONBLOCK;
        fcntl(socket_fd, F_SETFL, flags);
}

static int socket_check_writable(int socket_fd, int ms)
{
        fd_set fdw;
        FD_ZERO(&fdw);
        FD_SET(socket_fd, &fdw);

        struct timeval tv;
        tv.tv_sec  = ms / 1000;
        tv.tv_usec = (ms % 1000) * 1000;

        int ret = select(socket_fd + 1, NULL, &fdw, NULL, &tv);
        if (ret < 0) {
                return SOCKET_FAIL;
        } else if (ret == 0) {
                return SOCKET_TIMEOUT;
        }

        if (FD_ISSET(socket_fd, &fdw)) {
                return SOCKET_OK;
        }

        return SOCKET_FAIL;
}

static int socket_check_readable(int socket_fd, int ms)
{
        fd_set fdr;
        FD_ZERO(&fdr);
        FD_SET(socket_fd, &fdr);

        struct timeval tv;
        tv.tv_sec  = ms / 1000;
        tv.tv_usec = (ms % 1000) * 1000;

        int ret = select(socket_fd + 1, &fdr, NULL, NULL, &tv);
        if (ret < 0) {
                return SOCKET_FAIL;
        } else if (ret == 0) {
                return SOCKET_TIMEOUT;
        }

        if (FD_ISSET(socket_fd, &fdr)) {
                return SOCKET_OK;
        }

        return SOCKET_FAIL;
}

int socket_connect(int socket_fd, char *dest_addr, int dest_port)
{
    struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));

    struct hostent *host = NULL;
	if ((host = gethostbyname(dest_addr)) == NULL) {
        printf("Gethostname error, %s\n", strerror(errno));
		return -1;
	}

    sin.sin_family          = AF_INET;
    sin.sin_port            = htons((u_short)dest_port);
	//sin.sin_addr.s_addr     = inet_addr(dest_addr);
	memcpy(&sin.sin_addr, host->h_addr, host->h_length);

    if (connect(socket_fd, (struct sockaddr *)&sin, sizeof(sin)) == 0) {
            return SOCKET_OK;
    }

    switch (errno) {
    case EINPROGRESS:
    case EALREADY:
            break;
    default:
            usleep(100000);
            return SOCKET_FAIL;
    }

    int connect_timeout = 2000;
    if (socket_check_writable(socket_fd, connect_timeout)) {
            return SOCKET_FAIL;
    }

    int error = 0;
    socklen_t len = sizeof(error);
    getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len);

    if (error == 0) {
            return SOCKET_OK;
    }

    return SOCKET_FAIL;
}

int socket_write(int socket_fd, char *buf, int length, int ms)
{
        int sent_len = 0;
        int n = 0;
        int ret = 0;

        while (sent_len < length) {
                ret = socket_check_writable(socket_fd, ms);
                if (ret != SOCKET_OK) {
                        return ret;
                }

                n = send(socket_fd, buf + sent_len,
                                length - sent_len, 0);
                if (n == -1) {
                        return SOCKET_FAIL;
                } else if (n == 0) {
                        return SOCKET_FAIL;     //copy to send buffer fail
                }

                sent_len += n;
        }

        return SOCKET_OK;
}

int socket_read_length(int socket_fd, char *buf, int max_length, int ms)
{
        int length  = 0;
        int ret = 0;

        ret = socket_check_readable(socket_fd, ms);
        if (ret == SOCKET_OK) {
                length = recv(socket_fd, buf, max_length, 0);
        } else {
                return ret;
        }

        return length;
}

int socket_read(int socket_fd, char *buf, int req_length, int ms)
{
        char *ptr = (char *)buf;
        int rLen = 0;
        int n = 0;
        int ret = 0;

        while (rLen < req_length) {
                ret = socket_check_readable(socket_fd, ms);
                if (ret == SOCKET_OK) {
                        n = recv(socket_fd, ptr + rLen, req_length - rLen, 0);
                        if (n == -1) {
                                ret = SOCKET_FAIL;
                                break;
                        } else if (n == 0) {
                                ret = SOCKET_SHUTDOWN;
                                break;
                        }
                        rLen += n;
                } else {
                        break;
                }
        }

        return ret;
}

void socket_clear_recv_buffer(int socket_fd)
{
        int ret = 0;
        int buf_size = 256;
        char buf[buf_size];

        while (1) {
                ret = socket_read(socket_fd, buf, buf_size, 0);
                if ((ret == SOCKET_TIMEOUT)
                        || (ret == SOCKET_FAIL)
                        || (ret == SOCKET_SHUTDOWN)) {
                        break;
                }
        }
}

int socket_close(int socket_fd)
{
        if (socket_fd) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
        }

        return SOCKET_OK;
}

int socket_udpserver_create(int port)
{
        int udp_server_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udp_server_fd < 0) {
                return -1;
        }

        int share = 1;
        setsockopt(udp_server_fd, SOL_SOCKET, SO_REUSEADDR,
                        &share, sizeof(int));

        struct sockaddr_in servAddr;
        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family             = AF_INET;
        servAddr.sin_port               = htons(port);

        //Any IP address available locally
        servAddr.sin_addr.s_addr        = htonl(INADDR_ANY);

        if (bind(udp_server_fd, (struct sockaddr *)&servAddr,
                 sizeof(servAddr)) < 0) {
                return -1;
        }

        return udp_server_fd;
}

int socket_read_udp(int socket_fd, char *buf, int req_length, char *remote_addr)
{
        struct sockaddr_in peer_addr;
        socklen_t len = sizeof(peer_addr);

        int ret = socket_check_readable(socket_fd, DEFAULT_TIMEOUT);
        if (ret == SOCKET_OK) {
                int length = recvfrom(socket_fd, buf, req_length, 0,
                                        (struct sockaddr *)&peer_addr, &len);
                if (length < req_length) {
                        ret = SOCKET_FAIL;
                } else {
                        strcpy(remote_addr, inet_ntoa(peer_addr.sin_addr));
                        ret = SOCKET_OK;
                }
        }

        return ret;
}

static int socket_create_udp(void)
{
        int socket_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        return socket_fd;
}

int socket_multicast_write(char *multicast_ip, int multicast_port,
                           int local_port, char *buffer, int len)
{
        int multicast_fd = socket_create_udp();
        if (multicast_fd < 0) {
                return -1;
        }

        struct ifreq ifr;
        strcpy(ifr.ifr_name, "eth0");
        if (ioctl(multicast_fd, SIOCGIFADDR, &ifr) < 0) {
                return -1;
        }
        struct sockaddr_in *sa = (struct sockaddr_in *)(&ifr.ifr_addr);

        struct sockaddr_in peer_addr;
        struct sockaddr_in local_addr;
        memset(&peer_addr, 0, sizeof(struct sockaddr_in));
        peer_addr.sin_family    = AF_INET;
        peer_addr.sin_port      = htons(multicast_port);
        inet_pton(AF_INET, multicast_ip, &peer_addr.sin_addr);

        memset(&local_addr, 0, sizeof(struct sockaddr_in));
        local_addr.sin_family   = AF_INET;
        local_addr.sin_port     = htons(local_port);
        local_addr.sin_addr     = sa->sin_addr;

        sa = NULL;

        if (bind(multicast_fd, (struct sockaddr *)&local_addr,
                        sizeof(struct sockaddr_in)) == -1) {
                close(multicast_fd);
                return -1;
        }

        if (sendto(multicast_fd, buffer, len, 0, (struct sockaddr *)&peer_addr,
                                sizeof(struct sockaddr_in)) < 0) {
                close(multicast_fd);
                return -1;
        }

        close(multicast_fd);

        return 0;
}

int socket_multicast_server_init(char *multicast_ip,
                                 int multicast_port)
{
        int multicast_fd = socket_create_udp();
        if (multicast_fd < 0) {
                return -1;
        }

        struct ifreq ifr;
        strcpy(ifr.ifr_name, "eth0");
        if (ioctl(multicast_fd, SIOCGIFADDR, &ifr) < 0) {
                return -1;
        }

        struct sockaddr_in tmp_addr;
        memcpy(&tmp_addr, &(ifr.ifr_addr), sizeof(struct sockaddr_in));;

        int loop = 0;
        setsockopt(multicast_fd, IPPROTO_IP,
                        IP_MULTICAST_LOOP, &loop, sizeof(loop));

        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(struct ip_mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
        mreq.imr_interface.s_addr = inet_addr(inet_ntoa(tmp_addr.sin_addr));

        if (setsockopt(multicast_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                                sizeof(struct ip_mreq)) == -1) {
                close(multicast_fd);
                return -1;
        }

        struct sockaddr_in peer_addr;
        memset(&peer_addr, 0, sizeof(struct sockaddr_in));
        peer_addr.sin_family            = AF_INET;
        peer_addr.sin_port              = htons(multicast_port);
        peer_addr.sin_addr.s_addr       = htonl(INADDR_ANY);

        if (bind(multicast_fd, (struct sockaddr *)&peer_addr,
                        sizeof(struct sockaddr_in)) == -1) {
                close(multicast_fd);
                return -1;
        }

        return multicast_fd;
}

int socket_multicast_read(int multicast_fd, int multicast_port,
                          char *buf, int req_length, char *remote_addr)
{
        int ret = SOCKET_FAIL;

        struct sockaddr_in peer_addr;
        memset(&peer_addr, 0, sizeof(struct sockaddr_in));
        peer_addr.sin_family            = AF_INET;
        peer_addr.sin_port              = multicast_port;
        peer_addr.sin_addr.s_addr       = htonl(INADDR_ANY);

        unsigned int sock_len = sizeof(struct sockaddr_in);

        ret = socket_check_readable(multicast_fd, DEFAULT_TIMEOUT);
        if (ret == SOCKET_OK) {
                int length = recvfrom(multicast_fd, buf, req_length, 0,
                                        (struct sockaddr *)&peer_addr,
                                        &sock_len);
                if (length < req_length) {
                        ret = SOCKET_FAIL;
                } else {
                        strcpy(remote_addr, inet_ntoa(peer_addr.sin_addr));
                        ret = SOCKET_OK;
                }
        }

        return ret;
}

int socket_udp_broadcast(int broadcast_port, char *buffer, int len)
{
        int broadcast_fd = socket_create_udp();
        if (broadcast_fd < 0) {
                return -1;
        }

        struct sockaddr_in s_addr;
        struct ifreq ifr;
        int on = 1;

        strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
        if (ioctl(broadcast_fd, SIOCGIFBRDADDR, &ifr) == -1) {
                return -1;
        }

        s_addr.sin_family = AF_INET;
        s_addr.sin_port = htons(broadcast_port);

        struct sockaddr_in tmp_addr;
        memcpy(&tmp_addr, &(ifr.ifr_broadaddr), sizeof(struct sockaddr_in));;
        s_addr.sin_addr.s_addr = inet_addr(inet_ntoa(tmp_addr.sin_addr));

        if(setsockopt(broadcast_fd, SOL_SOCKET, SO_BROADCAST,
                                &on, sizeof(on)) < 0) {
                close(broadcast_fd);
                return -1;
        }

        int addr_len = sizeof(s_addr);
        if (sendto(broadcast_fd, buffer, len, 0,
                                (struct sockaddr *)&s_addr, addr_len) < 0) {
                close(broadcast_fd);
                return -1;
        }

        close(broadcast_fd);

        return 0;
}

int socket_udp_send_packet(char *ip, int port, char *buffer, int len)
{
        int udp_fd = socket_create_udp();
        if (udp_fd < 0) {
                return -1;
        }

        struct sockaddr_in local_addr;
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        struct ifreq ifr;
        strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
        if (ioctl(udp_fd, SIOCGIFBRDADDR, &ifr) == -1) {
                return -1;
        }

        local_addr.sin_port = htons(0);

        struct sockaddr_in tmp_addr;
        memcpy(&tmp_addr, &(ifr.ifr_broadaddr), sizeof(struct sockaddr_in));;
        local_addr.sin_addr.s_addr = inet_addr(inet_ntoa(tmp_addr.sin_addr));

        bind(udp_fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));

        struct sockaddr_in peer_addr;
        peer_addr.sin_family    = AF_INET;
        peer_addr.sin_port      = htons(port);
        peer_addr.sin_addr.s_addr = inet_addr(ip);

        sendto(udp_fd, buffer, len, 0, (struct sockaddr *)&peer_addr,
                        sizeof(struct sockaddr_in));

        close(udp_fd);

        return 0;
}
