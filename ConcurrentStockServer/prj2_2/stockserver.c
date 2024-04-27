/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#define SBUFSIZE 1024

typedef struct item *tree_link;
typedef struct item{
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
    sem_t w;
    tree_link left;
    tree_link right;
}ITEM;

ITEM* root;
int byte_cnt = 0;

sem_t file_mutex;

typedef struct {
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} sbuf_t;
sbuf_t sbuf;

void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear)%(sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front)%(sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}


void inorder(ITEM *ptr, FILE *fp);
void tree_free(ITEM *ptr);

void sig_int_handler(int sig)
{
    //stock.txt 갱신
    P(&file_mutex);
    FILE *fp = fopen("stock.txt", "w");
    inorder(root, fp);
    fclose(fp);
    V(&file_mutex);

    //free
    sbuf_deinit(&sbuf);
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

        P(&(ptr->mutex));
        ptr->readcnt++;
        if(ptr->readcnt == 1)
            P(&(ptr->w));
        V(&(ptr->mutex));

        fprintf(fp, "%d %d %d\n", ptr->ID, ptr->left_stock, ptr->price);

        P(&(ptr->mutex));
        ptr->readcnt--;
        if(ptr->readcnt == 0)
            V(&(ptr->w));
        V(&(ptr->mutex));

        inorder(ptr->right, fp);
    }
}

void inorder_show(char *stock_data, ITEM* ptr)
{
    char temp[100];
    if(ptr)
    {
        inorder_show(stock_data, ptr->left);

        P(&(ptr->mutex));
        ptr->readcnt++;
        if(ptr->readcnt == 1)
            P(&(ptr->w));
        V(&(ptr->mutex));

        sprintf(temp, "%d %d %d\n", ptr->ID, ptr->left_stock, ptr->price); //sprintf는 한줄 개행
        strcat(stock_data, temp);

        P(&(ptr->mutex));
        ptr->readcnt--;
        if(ptr->readcnt == 0)
            V(&(ptr->w));
        V(&(ptr->mutex));

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
        P(&(ptr->w));

        if(ptr->left_stock >= stock_amt) //재고가 있을 경우
        {
            ptr->left_stock -= stock_amt;
            Rio_writen(fd, "[buy] success\n", MAXBUF);
        }
        else    //재고가 없을 경우
        {
            Rio_writen(fd, "Not enough left stock\n", MAXBUF);
        }

        V(&(ptr->w));
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
        P(&(ptr->w));

        ptr->left_stock += stock_amt;
        Rio_writen(fd, "[sell] success\n", MAXBUF);

        V(&(ptr->w));
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
        Sem_init(&ptr->mutex, 0, 1);
        Sem_init(&ptr->w, 0, 1);
        ptr->readcnt = 0;
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

void *thread(void *vargp)
{
    int n;
    rio_t rio;
    char buf[MAXLINE];
    Pthread_detach(pthread_self());
    while(1)
    {
        int connfd = sbuf_remove(&sbuf);
        Rio_readinitb(&rio, connfd);
        while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
        {
            printf("server received %d bytes\n", n);

            if(!strcmp(buf, "exit\n"))
            {
                P(&file_mutex);
                FILE *fp = fopen("stock.txt", "w");
                inorder(root,fp);
                fclose(fp);
                V(&file_mutex);

                Rio_writen(connfd, "exit\n", MAXBUF);
                Close(connfd);
                break;
            }
            else if(!strcmp(buf, "\n"))
            {
                Rio_writen(connfd, "\n", MAXBUF);
            }
            else
                cmd_execute(connfd, buf);
        }
        if(n == 0)
        {
            P(&file_mutex);
            FILE *fp = fopen("stock.txt", "w");
            inorder(root, fp);
            fclose(fp);
            V(&file_mutex);

            Close(connfd);
            break;
        }
    }
}


int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;
     
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
        
    Signal(SIGINT, sig_int_handler);
    table_to_memory();
    Sem_init(&file_mutex, 0, 1);

    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf, SBUFSIZE);
    for(int i = 0; i < SBUFSIZE; i++)
        Pthread_create(&tid, NULL, thread, NULL);
    
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        sbuf_insert(&sbuf, connfd);
    }    
}
/* $end echoserverimain */
