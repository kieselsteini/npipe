 /* See LICENSE file for license details. */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 4096    /* how big lines can grow... */
#define VERSION "npipe 0.1.0 (C)2012 Sebastian Steinhauer"

void usage()
{
    fputs("usage: npipe [-h host] [-p port] [-f] [-V] [-v]\n", stderr);
    exit(EXIT_SUCCESS);
}

int dial(const char *host, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *res, *r;
    int             srv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &res) != 0) {
        fprintf(stderr, "error: cannot resolve hostname '%s': %s\n",
                host, strerror(errno));
        exit(EXIT_FAILURE);
    }
    for (r = res; r; r = r->ai_next) {
        srv = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
        if (srv == -1) continue;
        if (connect(srv, r->ai_addr, r->ai_addrlen) == 0) break;
        close(srv);
    }
    freeaddrinfo(res);
    if (! r) {
        fprintf(stderr, "error: cannot connect to host '%s'\n", host);
        exit(EXIT_FAILURE);
    }
    return srv;
}

int readline(int fd, size_t buf_len, char *buf)
{
    size_t  i = 0;
    char    c = 0;
    do {
        if (read(fd, &c, sizeof(char)) != sizeof(char))
            return -1;
        buf[i++] = c;
    } while (c != '\n' && i < buf_len);
    return i;
}

int main(int argc, char *argv[])
{
    const char  *host = NULL;
    const char  *port = NULL;
    int         i, maxfd, srv, in, out;
    fd_set      fd;
    int         verbose = 0, forking = 0;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') switch (argv[i][1]) {
            case 'V': verbose = 1; break;
            case 'f': forking = 1; break;
            case 'v': fprintf(stderr, "%s\n", VERSION); exit(EXIT_FAILURE);
            case 'h': if (++i < argc) host = argv[i]; break;
            case 'p': if (++i < argc) port = argv[i]; break;
            default: usage();
        }
    }
    if (! host || ! port) usage();

    /* do forking... */
    if (forking) {
        pid_t pid = fork();
        if (pid > 0) {
            signal(SIGCHLD, SIG_IGN); /* avoid zombies */
            printf("%d\n", pid);
            return EXIT_SUCCESS;
        } else if (pid < 0) {
            fputs("error: fork() failed", stderr);
            exit(EXIT_FAILURE);
        }
    }

    /* open socket */
    srv = dial(host, port);
    /* create FIFO */
    mkfifo("in", S_IRWXU);
    in = open("in", O_RDONLY | O_NONBLOCK, 0);
    if (in == -1) {
        fputs("error: cannot create FIFO in\n", stderr);
        exit(EXIT_FAILURE);
    }
    out = open("out", O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);
    if (out == -1) {
        fputs("error: cannot create file out\n", stderr);
        exit(EXIT_FAILURE);
    }
    /* main loop */
    for (;;) {
        maxfd = (srv > in ? srv : in) + 1;
        FD_ZERO(&fd);
        FD_SET(in, &fd);
        FD_SET(srv, &fd);
        i = select(maxfd, &fd, NULL, NULL, NULL);
        if (i < 0) {
            if (errno == EINTR) continue;
            fputs("error: select() failed\n", stderr);
            exit(EXIT_FAILURE);
        } else if (i == 0) continue;
        /* read from server */
        if (FD_ISSET(srv, &fd)) {
            char buf[BUFSIZE];
            i = readline(srv, sizeof(buf), buf);
            if (i > 0) {
                if (write(out, buf, i) != i) {
                    fputs("error: unable to write to out\n", stderr);
                    exit(EXIT_FAILURE);
                }
                if (verbose) printf("< %.*s", i, buf);
            } else {
                fputs("error: unable to read from host\n", stderr);
                exit(EXIT_FAILURE);
            }
        }
        /* read from pipe */
        if (FD_ISSET(in, &fd)) {
            char buf[BUFSIZE];
            i = readline(in, sizeof(buf), buf);
            if (i > 0) {
                if (write(srv, buf, i) != i) {
                    fputs("error: unable to send to host\n", stderr);
                    exit(EXIT_FAILURE);
                }
                if (verbose) printf("> %.*s", i, buf);
            } else {
                close(in);
                in = open("in", O_RDONLY | O_NONBLOCK, 0);
                if (in == -1) {
                    fputs("error: unable to reopen in\n", stderr);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    return EXIT_SUCCESS;
}

