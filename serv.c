#include "linux/init.h"
#include "linux/module.h"
#include "linux/kernel.h"
#include "linux/net.h"
#include "linux/in.h"
#include <linux/kthread.h>
#include <linux/slab.h>

/*
 * Server configuration
 */
#define SOCK_FAMILY     AF_INET
#define SOCK_TYPE       SOCK_STREAM
#define SOCK_PORT       2020
#define SOCK_ADDR       INADDR_ANY
#define SOCK_BACKLOG    5

struct socket *server;

void set_addr(struct sockaddr_in *str, uint8_t family, uint8_t port, uint64_t addr)
{
    str->sin_family = family;
    str->sin_port = htons(port);
    str->sin_addr.s_addr = htonl(addr);   
}


static int __init runserve(void)
{
    int err;
    struct sockaddr_in addr;

    server = (struct socket*)malloc(sizeof(struct socket));

    err = sock_create_kern(current->nsproxy->net_ns,SOCK_FAMILY,SOCK_TYPE, 0, &server);
    set_addr(&addr, SOCK_FAMILY, SOCK_PORT, SOCK_ADDR);

    kernel_bind(server, addr, sizeof(addr) );
    kernel_listen(server, SOCK_BACKLOG);

}  


static void __exit exitserve(void)
{
    kernel_sock_shutdown(server, SHUT_RDWR);
    sock_release(server);
}

