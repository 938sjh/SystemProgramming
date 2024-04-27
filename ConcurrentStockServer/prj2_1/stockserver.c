/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

typedef struct item *tree_link;
typedef struct item{
    int ID;
    int left_stock;
    int price;
    int readcnt;
    tree_link left;
    tree_link right;
}ITEM;

ITEM* root;

typedef struct _pool{
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
}pool;

int byte_cnt = 0;

void inorder(ITEM *ptr, FILE *fp);
void tree_free(ITEM *ptr);

void sig_int_handler(int sig)
{
    //stock.txt 갱신
    FILE *fp = fopen("stock.txt", "w");
    inorder(root, fp);
    fclose(fp);

    //tree free
    tree_free(root);

    exit(0);
}

void tree_free(ITEM *ptr)
{
    if(ptr)
    {
        tree_free(ptr->left);
        tree_free(ptr->right);
        free(ptr);
    }
}

void inorder(ITEM *ptr, FILE *fp)
{
    if(ptr)
    {
        inorder(ptr->left, fp);
        fprintf(fp, "%d %d %d\n", ptr->ID, ptr->left_stock, ptr->price);
        inorder(ptr->right, fp);
    }
}

void inorder_show(char *stock_data, ITEM* ptr)
{
    char temp[100];
    if(ptr)
    {
        inorder_show(stock_data, ptr->left);
        sprintf(temp, "%d %d %d\n", ptr->ID, ptr->left_stock, ptr->price); 
        strcat(stock_data, temp);
        inorder_show(stock_data, ptr->right);
    }
}

void show(int fd)
{
    char stock_data[MAXBUF];
    stock_data[0] = '\0';
    inorder_show(stock_data, root);
    Rio_writen(fd, stock_data, MAXBUF);
}

void buy(int fd, int stock_id, int stock_amt)
{
    ITEM* ptr = root;

    while(ptr)
    {
        if(ptr->ID == stock_id)
            break;
        else if(ptr->ID > stock_id)
            ptr = ptr->left;
        else
            ptr = ptr->right;
    }
    
    if(ptr == NULL) //stock_id가 존재하지 않을 경우
    {
        Rio_writen(fd, "No such stocks exist\n", MAXBUF);
    }
    else
    {
        if(ptr->left_stock >= stock_amt) //재고가 있을 경우
        {
            ptr->left_stock -= stock_amt;
            Rio_writen(fd, "[buy] success\n", MAXBUF);
        }
        else    //재고가 없을 경우
        {
            Rio_writen(fd, "Not enough left stock\n", MAXBUF);
        }
    }
}

void sell(int fd, int stock_id, int stock_amt)
{
    ITEM* ptr = root;

    while(ptr)
    {
        if(ptr->ID == stock_id)
            break;
        else if(ptr->ID > stock_id)
            ptr = ptr->left;
        else
            ptr = ptr->right;
    }

    if(ptr == NULL)
    {
        Rio_writen(fd, "No such stocks exist\n", MAXBUF);
    }
    else
    {
        ptr->left_stock += stock_amt;
        Rio_writen(fd, "[sell] success\n", MAXBUF);
    }
}

void cmd_execute(int fd, char *command)
{
    char *temp;
    char order[100];
    int stock_id, stock_amt;

    temp = strtok(command, " ");
    strcpy(order, temp);
    if(!strcmp(order, "show\n"))
    {
        show(fd);
    }
    else
    {
        temp = strtok(NULL, " ");
        stock_id = atoi(temp);
        temp = strtok(NULL, " ");
        stock_amt = atoi(temp);
        
        if(!strcmp(order, "buy"))
        {
            buy(fd, stock_id, stock_amt);
        }
        else if(!strcmp(order, "sell"))
        {
            sell(fd, stock_id, stock_amt);
        }
    }
}

ITEM* insertItem(ITEM* ptr, int stock_id, int stock_price, int left_stock)
{
    if(ptr == NULL)
    {
        ptr = (ITEM*)malloc(sizeof(ITEM));

        ptr->ID = stock_id;
        ptr->price = stock_price;
        ptr->left_stock = left_stock;
        ptr->left = NULL;
        ptr->right = NULL;
    }
    else if(stock_id < ptr->ID)
    {
        ptr->left = insertItem(ptr->left, stock_id, stock_price, left_stock);
    }
    else
    {
        ptr->right = insertItem(ptr->right, stock_id, stock_price, left_stock);
    }

    return ptr;
    //예외는 없음
}

void table_to_memory()
{
    FILE *fp = fopen("stock.txt", "r");
    int stock_id, stock_price, left_stock;
    
    while(fscanf(fp, "%d %d %d ", &stock_id, &left_stock, &stock_price) != EOF)
    {
        root = insertItem(root, stock_id, stock_price, left_stock);
    }
    fclose(fp);
}

void init_pool(int listenfd, pool *p)
{
    int i;
    p->maxi = -1;
    for(i = 0; i < FD_SETSIZE; i++)
        p->clientfd[i] = -1;

    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p)
{
    int i;
    p->nready--;
    for(i = 0; i < FD_SETSIZE; i++)
    {
        if(p->clientfd[i] < 0)
        {
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);

            FD_SET(connfd, &p->read_set);

            if(connfd > p->maxfd)
                p->maxfd = connfd;
            if(i > p->maxi)
                p->maxi = i;
            break;
        }
    }
    if (i == FD_SETSIZE)
        app_error("add_client error: Too many clients");
}

void check_clients(pool *p)
{
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++)
    {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        if((connfd > 0) && (FD_ISSET(connfd, &p->ready_set)))
        {
            p->nready--;
            if((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
            {
                byte_cnt += n;
                printf("server received %d bytes\n", n);
                if(!strcmp(buf, "exit\n"))
                {
                     FILE *fp = fopen("stock.txt", "w");
                     inorder(root, fp);
                     fclose(fp);
                    
                     Rio_writen(connfd, "exit\n", MAXBUF);                    
                     FD_CLR(connfd, &p->read_set);
                     p->clientfd[i] = -1;
                     Close(connfd);
                }
                else if(!strcmp(buf, "\n"))
                {
                    Rio_writen(connfd, "\n", MAXBUF);
                }
                else
                {
                    cmd_execute(connfd, buf);
                }
            }

            else
            {
                FILE *fp = fopen("stock.txt", "w");
                inorder(root, fp);
                fclose(fp);

                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }
        }
    }
}


int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static pool pool;
     
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    
    Signal(SIGINT, sig_int_handler);
    table_to_memory();

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);
    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &pool.ready_set))
        {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            add_client(connfd, &pool);
            Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
        }
        check_clients(&pool);
    }    
}
/* $end echoserverimain */
