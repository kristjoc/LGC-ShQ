#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "httpapp.h"

#define NET_SOFTERROR -1
#define NET_HARDERROR -2
#define POLL_READ 0
#define POLL_WRITE 1
#define MAX_CONNS 65535

int count = 4;

volatile sig_atomic_t running = 1;
static void sig_handler(int signum) {
        running = 0;
}
int uflag = 0, dflag = 0;
char url[64] = "127.0.0.1";
char ip_addr[16] = {0};
char host[32] = {0};
int port = 0;
int debug = 0;

static int main_ccn_client(char *_url)
{
        struct timespec start, end;
        char host[32]      = {0};
        char ip_addr[16]   = {0};
        char file_name[32] = {0};
        char header[1024]  = {0};
        int  sock = 0, port = 80, rc = 0, rd = 0;

        parse_url(_url, host, &port, file_name);

        get_ip_addr(host, ip_addr);
        if (strlen(ip_addr) == 0) {
                fprintf(stderr, "Bad IP address!\n");
                exit(EXIT_FAILURE);
        }

        sprintf(header, "GET %s HTTP/1.1\r\n"\
                "Accept: */*\r\n"\
                "User-Agent: My Wget Client\r\n"\
                "Host: %s\r\n"\
                "Connection: Keep-Alive\r\n"\
                "\r\n"\
                , file_name, host);

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip_addr);
        addr.sin_port = htons(port);

        /* clients connects to the server and starts the timer */
        clock_gettime(CLOCK_REALTIME, &start);
        rc = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
        if (rc < 0) {
                perror("connect");
                close(sock);
                exit(EXIT_FAILURE);
        }
        set_blocking(sock, false);

        /* client sends the GET/PUT header to the server */
        rc = write(sock, header, strlen(header));
        if (rc < 0) {
                perror("write");
                close(sock);
                exit(EXIT_FAILURE);
        }

        rc = poll_fd(sock, POLL_READ, 3000);
        if (rc <= 0) {
                fprintf(stderr, "ERROR during poll()\n");
                close(sock);
                exit(EXIT_FAILURE);
        }

        rd = download(sock, file_name, 0);

        shutdown(sock, SHUT_WR);
        close(sock);

        clock_gettime(CLOCK_REALTIME, &end);

        if (rd > 0)
                printf("%.4f\n", diff_time_ms(start, end));

        return 0;
}

static int prepare_client_socket(void)
{
        struct sockaddr_in caddr;
        int sock = 0, rc = 0;

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        if (strlen(ip_addr) == 0) {
                fprintf(stderr, "Bad IP address!\n");
                exit(EXIT_FAILURE);
        }

        memset(&caddr, 0, sizeof(caddr));
        caddr.sin_family = AF_INET;
        caddr.sin_addr.s_addr = inet_addr(ip_addr);
        caddr.sin_port = htons(port);

        rc = connect(sock, (struct sockaddr *) &caddr, sizeof(caddr));
        if (rc < 0) {
                perror("connect");
                close(sock);
                exit(EXIT_FAILURE);
        }

        set_blocking(sock, true);

        return sock;
}

static void *cthread_fn(void *data)
{
        int asock = *(int *)data;
        int rc = 0, ru = 0;
        char response[256] = {0};
        char header[1024]  = {0};
        char file_name[6];
        struct timespec start, end;
        char fsize2[4] = {'4','k','b','\0'};
        char fsize4[4] = {'8','k','b','\0'};
        char fsize8[5] = {'1','6','k','b','\0'};
        char fsize16[5] = {'3','2','k','b','\0'};
        char fsize32[5] = {'6','4','k','b','\0'};

        int fsize =(rand() % 5);
        switch (fsize) {
        case 0:
                memcpy(file_name, fsize2, 4);
                break;
        case 1:
                memcpy(file_name, fsize4, 4);
                break;
        case 2:
                memcpy(file_name, fsize8, 4);
                break;
        case 3:
                memcpy(file_name, fsize16, 5);
                break;
        case 4:
                memcpy(file_name, fsize32, 5);
                break;
        default:
                break;
        }

        pthread_t t_id = pthread_self();

        if (uflag) {
                struct stat buf;
                if (stat(file_name, &buf) < 0) {
                        fprintf(stderr, "'%s' file does not exist!\n", file_name);
                        exit(EXIT_FAILURE);
                }

                sprintf(header,                                         \
                        "PUT %s HTTP/1.1\r\n"                           \
                        "Content-length: %ld\r\n"                       \
                        "User-Agent: %ld\r\n"                           \
                        "Host: %s\r\n"                                  \
                        "Connection: Keep-Alive\r\n"                    \
                        "\r\n"                                          \
                        , file_name, (unsigned long)buf.st_size, t_id, host);
        }

        set_blocking(asock, false);

        clock_gettime(CLOCK_REALTIME, &start);

        /* client sends the GET/PUT header to the server */
        rc = write(asock, header, strlen(header));
        if (rc < 0) {
                /* perror("write"); */
                close(asock);
                exit(EXIT_FAILURE);
        }

        rc = poll_fd(asock, POLL_READ, 3000);
        if (rc <= 0) {
                /* fprintf(stderr, "ERROR during poll()\n"); */
                close(asock);
                exit(EXIT_FAILURE);
        }

        /* client read the HTTP response from server */
        rc = read(asock, response, 256);
        if (rc <= 0) {
                /* perror("read"); */
                close(asock);
                exit(EXIT_FAILURE);
        }

        /* client parses HTTP response headers */
        response[rc] = '\0';
        struct HTTP_RES_HEADER resp = parse_res_header(response);

        if (resp.status_code != 200) {
                printf("File cannot be downloaded: %d\n", resp.status_code);
                close(asock);
                exit(EXIT_FAILURE);
        }

        if (uflag) {
                ru = upload(asock, file_name, resp.content_length);
                rc = read(asock, response, 256);
                if (rc <= 0) {
                        /* perror("read"); */
                        close(asock);
                        exit(EXIT_FAILURE);
                }
        }

        shutdown(asock, SHUT_WR);
        close(asock);

        clock_gettime(CLOCK_REALTIME, &end);

        if ((uflag && ru > 0))
                printf(" %s,%.4f\n",file_name, diff_time_ms(start, end));

        return NULL;
}

static int main_client_fn(char *_url)
{
        char file_name[32] = {0};
        int rc = 0;
        pthread_t thread[MAX_CONNS];
        int asock[MAX_CONNS];
        time_t t;

        if (debug)
                printf("main client started ... \n");

        // Pasrse URL to get the IP, Port and Filename
        parse_url(_url, host, &port, file_name);
        get_ip_addr(host, ip_addr);
        if (strlen(ip_addr) == 0) {
                fprintf(stderr, "Bad IP address!\n");
                exit(EXIT_FAILURE);
        }

        // Launch all sockets
        int id = 0;
        for (id = 0; id < count; ++id) {
                asock[id] = prepare_client_socket();
                usleep(1000);
        }

        if (debug)
                printf("%d socket prepared\n", id);

        /* Intializes random number generator */
        srand((unsigned) time(&t));

        // Spawn all client threads
        for (int id = 0; id < count; ++id) {
                rc = pthread_create(&thread[id], NULL, cthread_fn, (void *)&asock[id]);
                if (rc < 0) {
                        /* perror("pthread_create"); */
                        close(asock[id]);
                }
                usleep(1000);
        }

        // Finally, Join threads and exit
        for (int id = 0; id < count; ++id)
                pthread_join(thread[id], NULL);

        return 0;
}

static int sock_srv_listen(int _port)
{
        struct sockaddr_in saddr;
        int sock = 0, opt = 1;
        /* open socket descriptor */
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        /* allows us to restart server immediately */
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(int));

        /* bind port to socket */
        bzero((char *) &saddr, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        saddr.sin_port = htons((unsigned short)_port);

        if (bind(sock, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
                perror("bind");
                close(sock);
                exit(EXIT_FAILURE);
        }

        /* get us ready to accept connection requests */
        if (listen(sock, MAX_CONNS) < 0) {
                perror("listen");
                close(sock);
                exit(EXIT_FAILURE);
        }

        return sock;
}

static void *sthread_fn(void *data)
{
        struct timespec start, end;
        int sock = *(int *)data;
        unsigned long file_size;
        char response[256] = {0};
        char file_name[32] = {0};
        char request[2048] = {0};
        int rc = 0;

        clock_gettime(CLOCK_REALTIME, &start);

        /* Uncomment this to log the fct at the server side */
        /* FILE *fp = NULL; */
        /* fp = fopen("/home/ocarina/kristjoc/fs.dat", "ab+"); */
        /* if (NULL == fp) { */
        /*     perror("fopen"); */
        /*     close(csock); */
        /*     return NULL; */
        /* } */

        set_blocking(sock, false);

        rc = poll_fd(sock, POLL_READ, 3000);
        if (rc <= 0) {
                /* perror("poll_fd(READ)"); */
                close(sock);
                return NULL;
        }

        rc = read(sock, request, 2048);
        if (rc <= 0) {
                /* perror("read"); */
                close(sock);
                return NULL;
        }
        request[rc] = '\0';

        file_size = parse_request(request, file_name);
        if (file_size > 0) {
                sprintf(response, \
                        "HTTP/1.1 200 OK\n"\
                        "Content-length: %ld\n"\
                        "\r\n"\
                        ,file_size);
        } else {
                sprintf(response, \
                        "HTTP/1.1 404 Not Found\n"\
                        "Content-length: %ld\n"\
                        "\r\n"\
                        ,file_size);
        }

        rc = write(sock, response, strlen(response));
        if (rc < 0) {
                perror("write");
                close(sock);
                return NULL;
        }

        if (dflag) {
                rc = poll_fd(sock, POLL_READ, 3000);
                if (rc <= 0) {
                        /* perror("poll_fd(READ)"); */
                        close(sock);
                        return NULL;
                }

                rc = read(sock, response, 256);
                if (rc <= 0) {
                        /* perror("read"); */
                        close(sock);
                        return NULL;
                }

                if (upload(sock, file_name, file_size) < 0) {
                        perror("upload");
                        close(sock);
                        return NULL;
                }
                rc = read(sock, response, 256);
                if (rc <= 0) {
                        /* perror("read"); */
                        close(sock);
                        return NULL;
                }

                shutdown(sock, SHUT_RDWR);
                close(sock);

                clock_gettime(CLOCK_REALTIME, &end);

                /* if (ru > 0) */
                /*     fprintf(fp, "%.4f\n", diff_time_ms(start, end)); */
                /* fclose(fp); */

                return NULL;
        } else if (uflag) {
                if (download(sock, file_name, file_size) < 0) {
                        perror("download");
                        close(sock);
                        return NULL;
                }
                rc = write(sock, "DONE", 5);
                if (rc < 0) {
                        perror("write");
                        close(sock);
                        return NULL;
                }
        }
        return NULL;
}

static void main_server_fn(int _port)
{
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        pthread_t th_id[MAX_CONNS];
        int sock, csock, rc;
        int th_nr = 0;

        sock = sock_srv_listen(_port);

        while (running) {
                /* wait for a connection request */
                csock = accept(sock, (struct sockaddr *) &caddr, &clen);
                if (csock < 0) {
                        perror("accept");
                        running = 0;
                        break;
                }

                if (th_nr >= MAX_CONNS) {
                        perror("Max connection reached");
                        close(csock);
                        running = 0;
                        break;
                }

                rc = pthread_create(&th_id[th_nr++], NULL, sthread_fn, (void *)&csock);
                if (rc < 0) {
                        perror("pthread_create");
                        close(csock);
                }
        }

        for (int i = 0; i < th_nr; i++)
                pthread_join(th_id[i], NULL);

        close(sock);
}

/******************************************************************************
 * MAIN
 ******************************************************************************/
int main(int argc, char *argv[])
{
        bool cflag = false, sflag = false, icn_flag = false;
        struct sigaction sa;
        int opt, rc;

        /* Set some signal handler */
        /* Ignore SIGPIPE
         * allow the server main thread to continue even after the client ^C */
        signal(SIGPIPE, SIG_IGN);

        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = sig_handler;
        rc = sigaction(SIGINT, &sa, NULL);
        if (rc) {
                perror("sigaction(SIGINT)");
                exit(EXIT_FAILURE);
        }
        rc = sigaction(SIGTERM, &sa, NULL);
        if (rc) {
                perror("sigaction(SIGTERM)");
                exit(EXIT_FAILURE);
        }

        while ((opt = getopt(argc, argv, "i:usc:dp:n:v")) != -1) {
                switch (opt) {
                case 's':
                        if (cflag) {
                                fprintf(stderr, "%s can't run both as client & server\n",
                                        argv[0]);
                                exit(EXIT_FAILURE);
                        }
                        sflag = true;
                        break;
                case 'c':
                        if (strchr(optarg, '-') != NULL) {
                                fprintf(stderr, "[-c url] -- wrong option\n");
                                exit(EXIT_FAILURE);
                        }
                        if (sflag) {
                                fprintf(stderr, "%s cannot run both as client and server\n",
                                        argv[0]);
                                exit(EXIT_FAILURE);
                        }
                        cflag = true;
                        if (strstr(optarg, "http"))
                                strcpy(url, optarg);
                        else {
                                fprintf(stderr, "URL should start with http\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'i':
                        if (strchr(optarg, '-') != NULL) {
                                fprintf(stderr, "[-i url] -- wrong option\n");
                                exit(EXIT_FAILURE);
                        }
                        if (sflag || cflag) {
                                fprintf(stderr, "%s cannot run as client/server in ICN mode\n",
                                        argv[0]);
                                exit(EXIT_FAILURE);
                        }
                        icn_flag = true;
                        if (strstr(optarg, "http"))
                                strcpy(url, optarg);
                        else {
                                fprintf(stderr, "URL should start with http\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'p':
                        if (strchr(optarg, '-') != NULL) {
                                fprintf(stderr, "[-p port] -- wrong option\n");
                                exit(EXIT_FAILURE);
                        }
                        port = atoi(optarg);
                        break;
                case 'u':
                        uflag = 1;
                        break;
                case 'd':
                        dflag = 1;
                        break;
                case 'n':
                        if (strchr(optarg, '-') != NULL) {
                                fprintf(stderr, "[-n count] -- wrong option\n");
                                exit(EXIT_FAILURE);
                        }
                        count = atoi(optarg);
                        break;
                case 'v':
                        debug = 1;
                        break;
                default: /* '?' */
                        fprintf(stderr, "Usage: %s\n"
                                "[-i  url (ccn scenario)]\n"
                                "[-s] server\n"
                                "[-c  url]\n"
                                "[-p  server listening port]\n"
                                "[-u] upload\n"
                                "[-d] download\n"
                                "[-v] with-debug\n"
                                "[-n count]\n", argv[0]);

                        exit(EXIT_FAILURE);
                }
        }

        if (optind < argc) {
                fprintf(stderr, "Expected argument after option %s\n", argv[optind]);
                exit(EXIT_FAILURE);
        }
        if (!cflag && !sflag && !icn_flag) {
                fprintf(stderr, "No running mode selected!\n");
        }

        if (icn_flag) {
                // CCN scenario
                main_ccn_client(url);
        } else if (cflag) {
                // Client case
                if (!uflag && !dflag) {
                        fprintf(stderr, "Download or Upload flag is mandatory!\n");
                        exit(EXIT_FAILURE);
                }
                if (chdir("/var/www/web/")) {
                        if (debug)
                                fprintf(stderr, "/var/www/web/ dir does not exist!\n");
                        exit(EXIT_FAILURE);
                }
                main_client_fn(url);
        } else if (sflag) {
                // Server case
                daemonize();
                main_server_fn(port);
        }

        exit(EXIT_SUCCESS);
}
