#include "linux/init.h"
#include "linux/module.h"
#include "linux/kernel.h"
#include "linux/net.h"
#include "linux/in.h"
#include <linux/kthread.h>
#include <linux/slab.h>
#include <net/net_namespace.h>

#define DAEMON_NAME     "CLIENT"
/*
 * Client configuration
 */
#define SOCK_FAMILY     AF_INET
#define SOCK_TYPE       SOCK_STREAM
#define SOCK_PORT       2020
#define SOCK_ADDR       INADDR_LOOPBACK
#define SOCK_BACKLOG    5
#define BUFF_SIZE       2048


struct socket * client;


static int get_request(struct socket *sock, unsigned char *buf, size_t size){

    struct msghdr msg;
    struct kvec vec;
    int length;

    //kvec setting
    vec.iov_len = size;
    vec.iov_base = buf;

    //msghdr setting
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    printk(DAEMON_NAME ": start get response\n");
    //get msg
    length = kernel_recvmsg(sock, &msg, &vec, size, size, msg.msg_flags);

    printk(DAEMON_NAME ": get request = \"%s\" \n", buf);

    return length;
}

static int send_request(struct socket *sock, unsigned char *buf, size_t size){

    int length;
    struct kvec vec; 
    struct msghdr msg;


    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    vec.iov_base = buf;
    vec.iov_len = strlen(buf);

    printk(DAEMON_NAME ": start send request.\n"); 
    
    length = kernel_sendmsg(sock, &msg, &vec, strlen(buf), strlen(buf));
    
    printk(DAEMON_NAME ": send request = %s\n", buf);

    return length;
}


static void set_addr(struct sockaddr_in *str, uint8_t family, uint8_t port, uint64_t addr)
{
    // protocol family
    str->sin_family = family;
    // port   
    str->sin_port = htons(port);
    // adress
    str->sin_addr.s_addr = htonl(addr);
}

static int __init runclient(void)
{
    int err;
    struct sockaddr_in addr;
    char *buffer = NULL;
    
    printk(KERN_INFO "Client is launched!");

    buffer = kmalloc(sizeof(char) * BUFF_SIZE, GFP_KERNEL);

    set_addr(&addr, SOCK_FAMILY, SOCK_PORT, SOCK_ADDR);

    err = sock_create_kern(&init_net,SOCK_FAMILY, SOCK_TYPE, 0, &client);
    if (err)
    {
        printk(KERN_ERR "Failed to create socket\n");
        return -1;
    }

    err = kernel_connect(client, (struct sockaddr *) &addr, sizeof(addr), NULL);
    if (err)
    {
        printk(KERN_ERR "Failed to establish connection\n");
        //return -1;
    }

    strcpy(buffer, "Hello world!");
    send_request(client,buffer, 0);
    get_request(client, buffer, BUFF_SIZE);


    kfree(buffer);
}



static void __exit exitclient(void)
{
    kernel_sock_shutdown(client, SHUT_RDWR);
    sock_release(client);
    printk(KERN_INFO "Client is closed!/n");

}


module_init(runclient)
module_exit(exitclient)