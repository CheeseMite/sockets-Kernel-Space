#include "linux/init.h"
#include "linux/module.h"
#include "linux/kernel.h"
#include "linux/net.h"
#include "linux/in.h"
#include <linux/kthread.h>
#include <linux/slab.h>
#include <net/net_namespace.h>
#include <asm/current.h>

#define DAEMON_NAME     "ECHO_SERVER"
/*
 * Server configuration
 */
#define SOCK_FAMILY     AF_INET
#define SOCK_TYPE       SOCK_STREAM
#define SOCK_PORT       2020
#define SOCK_ADDR       INADDR_ANY
#define SOCK_BACKLOG    5
#define BUFF_SIZE       2048
#define NUM_THREADS     8

struct socket *server;
struct task_struct **workers; // 11 threads
struct task_struct *serv;
uint8_t connection;

static int get_request(struct socket *sock, char *buf, size_t size)
{
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

static int send_request(struct socket *sock, char *buf, size_t size)
{
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

static int process(void *arg)
{
	int err;
	int msg_length;
	struct sockaddr_in info;
	struct socket *sock = NULL;
	char *buf = NULL;
	char addr[128];

	// allocate memory for buffer
	buf = kmalloc(sizeof(char) * BUFF_SIZE, GFP_KERNEL);

	sock = (struct socket *)arg;

	err = kernel_getpeername(sock, (struct sockaddr *)&info);

	printk(DAEMON_NAME ": The adress of client : %pI4\n",
	       &info.sin_addr.s_addr); //ntohl

	while (true) {
		// try to get message
		msg_length = get_request(sock, buf, BUFF_SIZE);

		if (msg_length)
			// if length of message > 0 then send message
			send_request(sock, buf, BUFF_SIZE);
		else
			break;
	}

	kfree(buf);
	kernel_sock_shutdown(sock, SHUT_RDWR);
	sock_release(sock);
}

static int server_daemon(void *arg)
{
	int err = 0;
	struct socket *new_sock = NULL;
	struct socket *server = NULL;
	struct task_struct *worker = NULL;

	printk(KERN_INFO "Server daemon launched!\n");

	server = (struct socket *)arg;

	while (!kthread_should_stop()) {
		err = kernel_accept(server, &new_sock, NULL);

		if (err) {
			printk(KERN_INFO "Failed attempting kernel_accept \n");
			continue;
		}
		//id = get_free_worker();
		//wake_up_process(workers[id]);

		// create new thread for each client
		worker = kthread_run(process, (void *)new_sock, "connection%d",
				     connection++);
	}

	return 0;
}

static void set_addr(struct sockaddr_in *str, uint8_t family, uint8_t port,
		     uint64_t addr)
{
	// protocol family
	str->sin_family = family;
	// port
	str->sin_port = htons(port);
	// adress
	str->sin_addr.s_addr = htonl(addr);
}

static int __init runserver(void)
{
	//int i;
	int err;
	struct sockaddr_in addr;

	// creating socket, init_net is a network namespace( net/net_namespace.h)
	err = sock_create_kern(&init_net, SOCK_FAMILY, SOCK_TYPE, 0, &server);
	printk(KERN_INFO "Server socket created\n");

	// build the adress struct
	set_addr(&addr, SOCK_FAMILY, SOCK_PORT, SOCK_ADDR);

	// bind socket to certain adress
	err = kernel_bind(server, (struct sockaddr *)&addr, sizeof(addr));
	printk(KERN_INFO "Socket is binded\n");

	// start listening
	err = kernel_listen(server, SOCK_BACKLOG);
	printk(KERN_INFO "Start listening\n");

	/*for (i = 0; i < 10; i++)
    {
        workers[0] = kthread_create(&process, &server,'worker');
    }*/

	// create and run new thread
	serv = kthread_run(server_daemon, (void *)server, "server daemon");

	return 0;
}

static void __exit exitserver(void)
{
	kernel_sock_shutdown(server, SHUT_RDWR);
	sock_release(server);
}

module_init(runserver) 
module_exit(exitserver)