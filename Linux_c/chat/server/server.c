#include"chat.h"
#include"pool.h"

int main()
{
    int i=0;
    int optval;
    char need[1000];
	int sock_fd,conn_fd;
	socklen_t len;
    struct sockaddr_in cli_addr,serv_addr;
    
    PACK recv_t;
    PACK *recv_pack; 
    int ret;

    int epfd,nfds;   
    struct epoll_event ev, events[MAX_EVENTS];  
    len = sizeof(struct sockaddr_in);

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    
    if (mysql_init(&mysql) == NULL) 
    {
        printf("mysql_init(): %s\n", mysql_error(&mysql));
        return -1;
    }

    if (mysql_real_connect(&mysql,"127.0.0.1","root","new_password","chat",0,NULL,0) == NULL) 
    {
        printf("mysql_real_connect(): %s\n", mysql_error(&mysql));
        return -1;
    }
    
    mysql_set_character_set(&mysql,"utf8");

    printf("服务器启动中...\n");

    sock_fd = socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd < 0)
        my_err("socket",__LINE__);
    printf("创建套接字...\n");

    optval = 1;
    if(setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,(void *)&optval,sizeof(int)) < 0)
        my_err("setsockopt",__LINE__);

    memset(&serv_addr,0,len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock_fd,(struct sockaddr *)&serv_addr,len) < 0)
        my_err("bind",__LINE__);
    printf("绑定端口...\n");

    if(listen(sock_fd,LISTENQ) < 0)
        my_err("listen",__LINE__);
    printf("侦听套接字...\n");

    epfd = epoll_create(MAX_EVENTS);   
    ev.data.fd = sock_fd;             
    ev.events = EPOLLIN;     
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);
    
    printf("创建线程池...\n");
    pool_init();
    sleep(1);

    printf("服务器启动成功！\n");
    printf("等待客户端的接入中...\n");

    pHead = U_read();
    User *t = pHead;
    pStart = R_read();
    pRec = RC_read();

    while(1)
    {
        nfds = epoll_wait(epfd, events, MAX_EVENTS, 1000);    

        for(i = 0; i < nfds; i++)
        {
            if(events[i].data.fd == sock_fd)
            {
                conn_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &len);
                printf("Connected: %s, fd is %d\n",inet_ntoa(cli_addr.sin_addr), conn_fd);
                ev.data.fd = conn_fd;               
                ev.events = EPOLLIN;                
                epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev);   
                continue;
            }
            else if(events[i].events & EPOLLIN)
            {
                ret = recv(events[i].data.fd, &recv_t, sizeof(PACK), MSG_WAITALL);
                recv_t.data.send_fd = events[i].data.fd;

                if(ret < 0)
                {
                    close(events[i].data.fd);
                    perror("revc");
                    continue;
                }
                else if(ret == 0)
                {
                    ev.data.fd = events[i].data.fd;
                    while(t)
                    {
                        if(t->fd == ev.data.fd)
                        {
                            t->statu_s = OFFLINE;
                            break;
                        }
                        t = t->next;
                    }
                    printf("fd: %d 的客户端退出\n",ev.data.fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                    close(events[i].data.fd);
                    continue;
                }

                printf("\n\e[1;34m****PACK****\e[0m\n");
                printf("\e[1;34m*\e[0m type      : %d\n", recv_t.type);
                printf("\e[1;34m*\e[0m send_fd   : %d\n", recv_t.data.send_fd);
                printf("\e[1;34m*\e[0m recv_fd   : %d\n", recv_t.data.recv_fd);
                printf("\e[1;34m*\e[0m send_name : %s\n", recv_t.data.send_name);
                printf("\e[1;34m*\e[0m recv_name : %s\n",recv_t.data.recv_name);
                printf("\e[1;34m*\e[0m mes       : %s\n", recv_t.data.mes);
                printf("\e[1;34m*************\e[0m\n\n");
                
                recv_pack = (PACK *)malloc(sizeof(PACK));
                memcpy(recv_pack, &recv_t, sizeof(PACK));
                

                pool_add(deal, (void *)recv_pack);        
                
            }
        }
    }

    pool_destroy();
    DeleteLink();
    DeleteLink_R();
    DeleteLink_RC();
    free(recv_pack);
    close(sock_fd);
    close(epfd);

    return 0;
}

User *U_read()
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char need[1000];
    int rows;
    int fields;

    User *pEnd, *pNew;

    sprintf(need, "select * from user");
    mysql_real_query(&mysql, need, strlen(need));
    res = mysql_store_result(&mysql);
    rows = mysql_num_rows(res);
    fields = mysql_num_fields(res);

    while(row = mysql_fetch_row(res))
    {
        pNew = (User *)malloc(sizeof(User));
        strcpy(pNew->name, row[0]);
        strcpy(pNew->passwd, row[1]);
        pNew->statu_s = OFFLINE;
        pNew->next = NULL;
        if(pHead == NULL)
            pHead = pNew;
        else
            pEnd->next = pNew;
        pEnd = pNew;
    }
    return pHead;
}

Relation *R_read()
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char need[1000];
    int rows;
    int fields;

    Relation *pEnd, *pNew;

    sprintf(need, "select * from relation");
    mysql_real_query(&mysql, need, strlen(need));
    res = mysql_store_result(&mysql);
    rows = mysql_num_rows(res);
    fields = mysql_num_fields(res);

    while(row = mysql_fetch_row(res))
    {
        pNew = (Relation *)malloc(sizeof(Relation));
        strcpy(pNew->name1, row[0]);
        strcpy(pNew->name2, row[1]);
        pNew->statu_s = row[2][0] - '0';
        pNew->next = NULL;
        if(pStart == NULL)
            pStart = pNew;
        else
            pEnd->next = pNew;
        pEnd = pNew;
    }
    return pStart;
}

Recordinfo *RC_read()
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char need[1000];
    int rows;
    int fields;

    Recordinfo *pEnd, *pNew;

    sprintf(need, "select * from message");
    mysql_real_query(&mysql, need, strlen(need));
    res = mysql_store_result(&mysql);
    rows = mysql_num_rows(res);
    fields = mysql_num_fields(res);

    while(row = mysql_fetch_row(res))
    {
        pNew = (Recordinfo *)malloc(sizeof(Recordinfo));
        strcpy(pNew->name1, row[0]);
        strcpy(pNew->name2, row[1]);
        strcpy(pNew->message, row[2]);
        pNew->next = NULL;
        if(pRec == NULL)
            pRec = pNew;
        else
            pEnd->next = pNew;
        pEnd = pNew;
    }
    return pRec;
}

void *deal(void *recv_pack_t)
{
    PACK *recv_pack = (PACK *)recv_pack_t;
    switch(recv_pack->type)
    {
    case EXIT:
        Exit(recv_pack);
        break;

    case REGISTE:
        registe(recv_pack);            
        break;

    case LOGIN:
        login(recv_pack);
        break;

    case LOOK_FRI:
        look_fri(recv_pack);
        break;

    case GET_FRI_STA:
        get_fri_sta(recv_pack);
        break;

    case ADD_FRI:
        add_fri(recv_pack);
        break;

    case DEL_FRI:
        del_fri(recv_pack);
        break;

    case SHI_FRI:
        shi_fri(recv_pack);
        break;
        
    case REL_FRI:
        rel_fri(recv_pack);
        break;
        
    case CHAT_ONE:
        chat_one(recv_pack);
        break;
        
    case CHECK_MES_FRI:
        check_mes_fri(recv_pack);
        break;
        
    case CRE_GRP:
        cre_grp(recv_pack);
        break;

    case ADD_GRP:
        add_grp(recv_pack);
        break;

    case OUT_GRP:
        out_grp(recv_pack);
        break;

    case DEL_GRP:
        del_grp(recv_pack);
        break;

    case SET_GRP_ADM:
        set_grp_adm(recv_pack);
        break;

    case KICK_GRP:
        kick_grp(recv_pack);
        break;

    case CHECK_GRP:
        check_grp(recv_pack);
        break;

    case CHECK_MEM_GRP:
        check_mem_grp(recv_pack);
        break;

    case CHAT_MANY:
        chat_many(recv_pack);
        break;

    case CHECK_MES_GRP:
        check_mes_grp(recv_pack);
        break;

    case RECV_FILE:
        recv_file(recv_pack);
        break;

    case SEND_FILE:
        send_file(recv_pack);
        break;

    default:
        break;
    }
}

void Exit(PACK *recv_pack)
{
    User *t = pHead;
    while(t)
    {
        if(strcmp(t->name, recv_pack->data.send_name) == 0)
        {
            t->statu_s = OFFLINE;
            break;
        }
        t = t->next;
    }
    close(recv_pack->data.send_fd);
}

void registe(PACK *recv_pack)
{
    char need[1000];

    int a;
    char ch[5];
    int fd = recv_pack->data.send_fd;

    User *t = pHead;
    int flag_1 = 0;
    User *pNew = (User *)malloc(sizeof(User));
    while(t)
    {
        if(strcmp(t->name, recv_pack->data.send_name) == 0)
        {
            flag_1 = 1;
            break;
        }
        t = t->next;
    }

    if(flag_1 == 0)
    {
        strcpy(pNew->name, recv_pack->data.send_name);
        strcpy(pNew->passwd, recv_pack->data.mes);
        pNew->statu_s = OFFLINE;
        Insert(pNew);

        memset(need, 0, strlen(need));
        sprintf(need, "insert into user values('%s', '%s')", recv_pack->data.send_name, recv_pack->data.mes);
        mysql_real_query(&mysql, need, strlen(need));
        ch[0] = '1';
    }
    else
        ch[0] = '0';
    
    ch[1] = '\0';
    send_pack(fd, recv_pack, ch);
}

void Insert(User *pNew)
{
    User *t = pHead;
    while(t && t->next != NULL)
        t = t->next;
    t->next = pNew;
    pNew->next = NULL;
}

void login(PACK *recv_pack)
{
    char ch[5];
    int fd = recv_pack->data.send_fd;
    int i;

    User *t = pHead;
    int flag_1 = 0;
    while(t)
    {
        if(strcmp(t->name, recv_pack->data.send_name) == 0 && strcmp(t->passwd, recv_pack->data.mes) == 0)
        {
            flag_1 = 1;
            break;
        }
        t = t->next;
    }

    if(flag_1 == 0)
        ch[0] = '0';
    else
    {
        if(t->statu_s == OFFLINE)
        {
            ch[0] = '1';
            t->statu_s = ONLINE;
            t->fd = recv_pack->data.send_fd;
        }
        else 
            ch[0] = '2';
    }
    ch[1] = '\0';
    send_pack(fd, recv_pack, ch);
    for(i = 0; i < sign; i++)
    {
        if((ch[0] == '1') && strcmp(recv_pack->data.send_name, Mex_Box[i].data.recv_name) == 0 && (Mex_Box[i].type == CHAT_ONE))
        {
            send_mes(fd, CHAT_ONE, &Mex_Box[i], "1");
            book++;
        }
        if((ch[0] == '1') && strcmp(recv_pack->data.send_name, Mex_Box[i].data.send_name) == 0 && (Mex_Box[i].type == CHAT_MANY))
        {
            send_mes(fd, CHAT_MANY, &Mex_Box[i], "2");
            book++;
        }
        if((ch[0] == '1') && strcmp(recv_pack->data.send_name, Mex_Box[i].data.recv_name) == 0 && (Mex_Box[i].type == ADD_FRI))
        {
            pool_add(deal, (void *)&Mex_Box[i]);  
            book++;
        }
        if((ch[0] == '1') && strcmp(recv_pack->data.send_name, Mex_Box[i].data.send_name) == 0 && (Mex_Box[i].type == ADD_GRP))
        {
            pool_add(deal, (void *)&Mex_Box[i]);  
            book++;
        }
        if((ch[0] == '1') && strcmp(recv_pack->data.send_name, Mex_Box[i].data.mes) == 0)
        {
            send_mes(fd, Mex_Box[i].type, &Mex_Box[i], "6");
            book++;
        }
        if((ch[0] == '1') && strcmp(recv_pack->data.send_name, Mex_Box[i].data.recv_name) == 0 && strcmp(Mex_Box[i].data.mes, "ok") == 0)
        {
            send_file(&Mex_Box[i]);
            book++;
        }
    }
    if(book == sign)
        sign = book = 0;
}

void look_fri(PACK *recv_pack)
{
    int flag = LOOK_FRI;
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char need[1000];
    int rows;
    int i;

    int fd = recv_pack->data.send_fd;
    int statu_s;

    memset(need, 0, strlen(need));
    sprintf(need, "select * from relation where user='%s' or oth_user='%s'", recv_pack->data.send_name, recv_pack->data.send_name);
    mysql_real_query(&mysql, need, strlen(need));
    
    res = mysql_store_result(&mysql);
    
    rows = mysql_num_rows(res);

    if(rows == 0)
        recv_pack->fri_info.friends_num = 0;
    else
    {
        i = 0;
        while(row = mysql_fetch_row(res))
        {
            if(strcmp(row[0], recv_pack->data.send_name) == 0)
            {
                strcpy(recv_pack->fri_info.friends[i], row[1]);
                statu_s = row[2][0] - '0';
                recv_pack->fri_info.friends_status[i] = statu_s;
                i++;
            }
            else if(strcmp(row[1], recv_pack->data.send_name) == 0)
            {
                strcpy(recv_pack->fri_info.friends[i], row[0]);
                statu_s = row[2][0] - '0';
                recv_pack->fri_info.friends_status[i] = statu_s;
                i++;
            }   
        }
        recv_pack->fri_info.friends_num = i;
    }
    send_mes(fd, flag, recv_pack, "");
}

void get_fri_sta(PACK *recv_pack)
{
    int flag = GET_FRI_STA;
    char ch[5];
    int fd = recv_pack->data.send_fd;

    User *t = pHead;
    int flag_1 = 0;
    while(t)
    {
        if(strcmp(t->name, recv_pack->data.send_name) == 0)
        {
            flag_1 = 1;
            break;
        }
        t = t->next;
    }

    if(t->statu_s == OFFLINE)
        ch[0] = '0';
    else 
        ch[0] = '1';
    ch[1] = '\0';

    send_mes(fd, flag, recv_pack, ch);
}

void add_fri(PACK *recv_pack)
{
    char need[1000];

    int flag = ADD_FRI;
    int fd = recv_pack->data.send_fd;
    char ch[5];
    char ss[MAX_CHAR];

    User *t = pHead;
    int flag_2 = 0;
    Relation *q = pStart;
    int flag_1 = 0;
    Relation *pNew = (Relation *)malloc(sizeof(Relation));
    while(q)
    {
        if((strcmp(q->name1, recv_pack->data.recv_name) == 0 && strcmp(q->name2, recv_pack->data.send_name) == 0) || (strcmp(q->name1, recv_pack->data.send_name) == 0 && strcmp(q->name2, recv_pack->data.recv_name) == 0))
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }

    if(flag_1 == 1)
    {
        ch[0] = '4';
        send_mes(fd, flag, recv_pack, ch);
        free(pNew);
        pNew = NULL;
        return;
    }
    else
    {
        while(t)
        {
            if(strcmp(t->name, recv_pack->data.recv_name) == 0)
            {
                flag_2 = 1;
                break;
            }
            t = t->next;
        }
        
        if(flag_2 == 0)
        {
            ch[0] = '3';
            send_mes(fd, flag, recv_pack, ch);
            free(pNew);
            pNew = NULL;
            return;
        }
        else
        {
            if(t->statu_s != OFFLINE)
            {
                fd = t->fd;
                if(recv_pack->data.mes[0] == '0')
                    ch[0] = '0';
                else if(recv_pack->data.mes[0] == 'y')
                {
                    ch[0] = '1';
                    strcpy(pNew->name1, recv_pack->data.recv_name);
                    strcpy(pNew->name2, recv_pack->data.send_name);
                    pNew->statu_s = FRIEND;
                    Insert_R(pNew);

                    memset(need, 0, strlen(need));
                    sprintf(need, "insert into relation values('%s', '%s', %d)", recv_pack->data.recv_name, recv_pack->data.send_name, FRIEND);
                    mysql_real_query(&mysql, need, strlen(need));
                }
                else if(recv_pack->data.mes[0] == 'n')
                    ch[0] = '2';
                
                strcpy(ss,recv_pack->data.recv_name);
                strcpy(recv_pack->data.recv_name, recv_pack->data.send_name);
                strcpy(recv_pack->data.send_name, ss);
                send_mes(fd, flag, recv_pack, ch);
            }
            else if(t->statu_s == OFFLINE)
            {
                memcpy(&Mex_Box[sign++], recv_pack, sizeof(PACK));       
            }
        }
    }
}

void Insert_R(Relation *pNew)
{
    Relation *t = pStart;
    while(t && t->next != NULL)
        t = t->next;
    t->next = pNew;
    pNew->next = NULL;
}

void del_fri(PACK *recv_pack)
{
    char need[1000];

    int flag = DEL_FRI;
    char ch[5];
    int fd = recv_pack->data.send_fd;

    Relation *q = pStart;
    int flag_1 = 0;
    while(q)
    {
        if((strcmp(q->name1, recv_pack->data.mes) == 0 && strcmp(q->name2, recv_pack->data.send_name) == 0) || (strcmp(q->name1, recv_pack->data.send_name) == 0 && strcmp(q->name2, recv_pack->data.mes) == 0))
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }

    if(flag_1 == 0)
        ch[0] = '0';
    else
    {
        Delete_R(q);

        memset(need, 0, strlen(need));
        sprintf(need, "delete from relation where (user='%s' and oth_user='%s') or (user='%s' and oth_user='%s')", recv_pack->data.send_name, recv_pack->data.mes, recv_pack->data.mes, recv_pack->data.send_name);
        mysql_real_query(&mysql, need, strlen(need));
        ch[0] = '1';
    }
    send_mes(fd, flag, recv_pack, ch);
}

void Delete_R(Relation *pNew)
{
    Relation *t = pStart;
    Relation *ptr;
    while(t)
    {
        if((strcmp(t->name1, pNew->name1) == 0 && strcmp(t->name2, pNew->name2) == 0) || (strcmp(t->name1, pNew->name2) == 0 && strcmp(t->name2, pNew->name2) == 0))
        {
            if(pStart == t)
            {
                pStart = t->next;
                free(t);
                return;
            }
            ptr->next = t->next;
            free(t);
            return;
        }
        ptr = t;
        t = t->next;
    }
}

void shi_fri(PACK *recv_pack)
{
    char need[1000];

    int flag = SHI_FRI;
    char ch[5];
    int fd = recv_pack->data.send_fd;

    Relation *q = pStart;
    int flag_1 = 0;
    while(q)
    {
        if((strcmp(q->name1, recv_pack->data.mes) == 0 && strcmp(q->name2, recv_pack->data.send_name) == 0) || (strcmp(q->name1, recv_pack->data.send_name) == 0 && strcmp(q->name2, recv_pack->data.mes) == 0))
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }

    if(flag_1 == 0)
        ch[0] = '0';
    else
    {
        q->statu_s = FRI_BLK;
        memset(need, 0, strlen(need));
        sprintf(need, "update relation set status=%d where (user='%s' and oth_user='%s') or (user='%s' and oth_user='%s')", FRI_BLK, recv_pack->data.send_name, recv_pack->data.mes, recv_pack->data.mes, recv_pack->data.send_name);
        mysql_real_query(&mysql, need, strlen(need));
        ch[0] = '1';
    }
    send_mes(fd, flag, recv_pack, ch);

}

void rel_fri(PACK *recv_pack)
{
    char need[1000];

    int flag = REL_FRI;
    char ch[5];
    int fd = recv_pack->data.send_fd;

    Relation *q = pStart;
    int flag_1 = 0;
    while(q)
    {
        if((strcmp(q->name1, recv_pack->data.mes) == 0 && strcmp(q->name2, recv_pack->data.send_name) == 0) || (strcmp(q->name1, recv_pack->data.send_name) == 0 && strcmp(q->name2, recv_pack->data.mes) == 0))
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }

    if(flag_1 == 0)
        ch[0] = '0';
    else
    {
        q->statu_s = FRI_BLK;
        memset(need, 0, strlen(need));
        sprintf(need, "update relation set status=%d where (user='%s' and oth_user='%s') or (user='%s' and oth_user='%s')", FRIEND, recv_pack->data.send_name, recv_pack->data.mes, recv_pack->data.mes, recv_pack->data.send_name);
        mysql_real_query(&mysql, need, strlen(need));
        ch[0] = '1';
    }
    send_mes(fd, flag, recv_pack, ch);

}

void chat_one(PACK *recv_pack)
{
    int flag = CHAT_ONE;
    char ch[5];
    int fd = recv_pack->data.send_fd;
    char ss[MAX_CHAR];
    time_t now;
    char *str;
    
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char need[1000];
    int rows;
    int fields;
    RECORD_INFO rec_info[55];
    int i = 0,j;
    
    User *t = pHead;
    Relation *q = pStart;
    Recordinfo *p = pRec;
    int flag_2 = 0;
    int flag_1 = 0;

    Recordinfo *pNew = (Recordinfo *)malloc(sizeof(Recordinfo));

    if(strcmp(recv_pack->data.mes, "q") == 0)
    {
        while(t)
        {
            if(strcmp(t->name, recv_pack->data.send_name) == 0)
            {
                t->statu_s = ONLINE;
                t->chat[0] = '\0';
                free(pNew);
                pNew = NULL;
                return;
            }
            t = t->next;
        }
    }

    while(q)
    {
        if(((strcmp(q->name1,recv_pack->data.send_name) == 0 && strcmp(q->name2, recv_pack->data.recv_name) == 0) || (strcmp(q->name2,recv_pack->data.send_name) == 0 && strcmp(q->name1, recv_pack->data.recv_name) == 0)) && (q->statu_s == FRI_BLK))
        {
            ch[0] = '3';
            send_mes(fd, flag, recv_pack, ch);
            free(pNew);
            pNew = NULL;
            return;
        }
        q = q->next;
    }

    t = pHead;
    while(t)
    {
        if(strcmp(t->name, recv_pack->data.recv_name) == 0)
        {
            flag_2 = 1;
            break;
        }
        t = t->next;
    }

    if(flag_2 == 0)
    {
        ch[0] = '0';
        send_mes(fd, flag, recv_pack, ch);
        free(pNew);
        pNew = NULL;
        return;
    }
    else
    {
        if(recv_pack->data.mes[0] == '1')
        {
            memset(need, 0, strlen(need));
            sprintf(need, "select * from off_message where user='%s' and oth_user='%s'", recv_pack->data.recv_name, recv_pack->data.send_name);
            mysql_real_query(&mysql, need, strlen(need));
            res = mysql_store_result(&mysql);
            rows = mysql_num_rows(res);
            fields = mysql_num_fields(res);
            while(row = mysql_fetch_row(res))
            {
                strcpy(pNew->name1, row[0]);
                strcpy(pNew->name2, row[1]);
                strcpy(pNew->message, row[2]);
                Insert_RC(pNew);
                memset(need, 0, strlen(need));
                sprintf(need, "insert into message values('%s', '%s', '%s')", row[0], row[1], row[2]);
                mysql_real_query(&mysql, need, strlen(need));
                
                strcpy(recv_pack->rec_info[i].name1, row[0]);
                strcpy(recv_pack->rec_info[i].name2, row[1]);
                strcpy(recv_pack->rec_info[i].message, row[2]);
                i++;
                if(i > 50)
                    break;                          
            }
            recv_pack->rec_info[i].message[0] = '0';
            send_mes(fd, flag, recv_pack, "6");

            memset(need, 0, strlen(need));
            sprintf(need, "delete from off_message where user='%s' and oth_user='%s'", recv_pack->data.recv_name, recv_pack->data.send_name);
            mysql_real_query(&mysql, need, strlen(need));
            
            t = pHead;
            while(t)
            {
                if(strcmp(t->name, recv_pack->data.send_name) == 0)
                {
                    t->statu_s = ONE_CHAT;
                    strcpy(t->chat, recv_pack->data.recv_name);
                    break;
                }
                t = t->next;
            }
            t = pHead;
            while(t)
            {
                if(strcmp(t->name, recv_pack->data.recv_name) == 0 && (t->statu_s != OFFLINE))
                {
                    flag_1 = 1;
                    break;
                }
                t = t->next;
            }
            if(flag_1 == 1)
            {
                ch[0] = '1';
                fd = t->fd;
                strcpy(ss,recv_pack->data.recv_name);
                strcpy(recv_pack->data.recv_name, recv_pack->data.send_name);
                strcpy(recv_pack->data.send_name, ss);
                send_mes(fd, flag, recv_pack, ch);
            }
            else 
            {
                ch[0] = '2';
                send_mes(fd, flag, recv_pack, ch);
                memcpy(&Mex_Box[sign++], recv_pack, sizeof(PACK));
            }
        }
        else
        {
            t = pHead;
            while(t)
            {
                if(strcmp(t->name, recv_pack->data.recv_name) == 0 && strcmp(t->chat, recv_pack->data.send_name) == 0 && (t->statu_s == ONE_CHAT))
                {
                    fd = t->fd;
                    strcpy(pNew->name1, recv_pack->data.send_name);
                    strcpy(pNew->name2, recv_pack->data.recv_name);
                    strcpy(pNew->message, recv_pack->data.mes);
                    Insert_RC(pNew);

                    memset(need, 0, strlen(need));
                    sprintf(need, "insert into message values('%s', '%s', '%s')", recv_pack->data.send_name, recv_pack->data.recv_name, recv_pack->data.mes);
                    mysql_real_query(&mysql, need, strlen(need));
                    
                    memset(ss, 0, MAX_CHAR);
                    strcpy(ss,recv_pack->data.recv_name);
                    strcpy(recv_pack->data.recv_name, recv_pack->data.send_name);
                    time(&now);
                    str = ctime(&now);
                    str[strlen(str) - 1] = '\0';
                    memcpy(recv_pack->data.send_name, str, strlen(str));
                    send_mes(fd, flag, recv_pack, recv_pack->data.mes);
                    return;
                }
                else if(strcmp(t->name, recv_pack->data.recv_name) == 0 && strcmp(t->chat, recv_pack->data.send_name) != 0)
                {
                    memset(need, 0, strlen(need));
                    sprintf(need, "insert into off_message values('%s', '%s', '%s')", recv_pack->data.send_name, recv_pack->data.recv_name, recv_pack->data.mes);
                    mysql_real_query(&mysql, need, strlen(need));
                    free(pNew);
                    pNew = NULL;
                    return;
                }
                t = t->next;
            }
        }
    }
}

void Insert_RC(Recordinfo *pNew)
{
    Recordinfo *p = pRec;
    while(p && p->next != NULL)
        p = p->next;
    p->next = pNew;
    pNew->next = NULL;
}

void check_mes_fri(PACK *recv_pack)
{
    int i = 0;
    int flag = CHECK_MES_FRI;
    char ch[5];
    int fd = recv_pack->data.send_fd;
    Relation *q = pStart;
    Recordinfo *p = pRec;
    int flag_1 = 0;
    while(q)
    {
        if(((strcmp(q->name1, recv_pack->data.send_name) == 0 && strcmp(q->name2, recv_pack->data.mes) == 0) || (strcmp(q->name2, recv_pack->data.send_name) == 0 && strcmp(q->name1, recv_pack->data.mes) == 0)) && (q->statu_s == FRIEND)) 
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }
    if(flag_1 == 0)
        ch[0] = '0';
    else
    {
        ch[0] = '1';
        while(p)
        {
            if((strcmp(p->name1, recv_pack->data.send_name) == 0 && strcmp(p->name2, recv_pack->data.mes) == 0) || (strcmp(p->name2, recv_pack->data.send_name) == 0 && strcmp(p->name1, recv_pack->data.mes) == 0))
            {
                strcpy(recv_pack->rec_info[i].name1, p->name1);
                strcpy(recv_pack->rec_info[i].name2, p->name2);
                strcpy(recv_pack->rec_info[i].message, p->message);
                i++;
                if(i > 50)
                    break;
            }
            p = p->next;
        }
    }
    recv_pack->rec_info[i].message[0] = '0';
                            
    send_mes(fd, flag, recv_pack, ch);
}

void cre_grp(PACK *recv_pack)
{
    char need[1000];

    int flag = CRE_GRP;
    int fd = recv_pack->data.send_fd;
    char ch[5];

    Relation *q = pStart;
    int flag_1 = 0;
    Relation *pNew = (Relation *)malloc(sizeof(Relation));
    while(q)
    {
        if(strcmp(q->name2, recv_pack->data.mes) == 0)
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }

    if(flag_1 == 1)
    {
        ch[0] = '0';
    }
    else
    {
        ch[0] = '1';
        strcpy(pNew->name1, recv_pack->data.send_name);
        strcpy(pNew->name2, recv_pack->data.mes);
        pNew->statu_s = GRP_OWN;
        Insert_R(pNew);
        
        memset(need, 0, strlen(need));
        sprintf(need, "insert into relation values('%s', '%s', %d)", recv_pack->data.send_name, recv_pack->data.mes, GRP_OWN);
        mysql_real_query(&mysql, need, strlen(need));
    }
    send_mes(fd, flag, recv_pack, ch);
}

void add_grp(PACK *recv_pack)
{
    char need[1000];

    int flag = ADD_GRP;
    int fd = recv_pack->data.send_fd;
    char ch[5];

    User *t = pHead;
    Relation *q = pStart;
    int flag_1 = 0;
    Relation *pNew = (Relation *)malloc(sizeof(Relation));
    if(strcmp(recv_pack->data.mes, "y") == 0)
    {   
        while(t)
        {
            if(strcmp(t->name, recv_pack->data.recv_name) == 0)
            {
                fd = t->fd;
                break;
            }
            t = t->next;
        }
        ch[0] = '2';
        printf("%s\n", recv_pack->file.mes);
        strcpy(pNew->name1, recv_pack->data.recv_name);
        strcpy(pNew->name2, recv_pack->data.send_name);
        pNew->statu_s = GRP;
        Insert_R(pNew);

        memset(need, 0, strlen(need));
        sprintf(need, "insert into relation values('%s', '%s', %d)", recv_pack->data.recv_name, recv_pack->data.send_name, GRP);
        mysql_real_query(&mysql, need, strlen(need));
        send_mes(fd, flag, recv_pack, ch);
        return;
    }
    else if(strcmp(recv_pack->data.mes, "n") == 0)
    {
        while(t)
        {
            if(strcmp(t->name, recv_pack->data.recv_name) == 0)
            {
                fd = t->fd;
                break;
            }
            t = t->next;
        }
        ch[0] = '3';
        send_mes(fd, flag, recv_pack, ch);
        return;
    }
    while(q)
    {
        if(strcmp(q->name2, recv_pack->data.mes) == 0 && (q->statu_s == GRP_OWN))
        {
            flag_1 = 1;
            strcpy(recv_pack->data.recv_name, q->name1);
            break;
        }
        q = q->next;
    }

    if(flag_1 == 0)
    {
        ch[0] = '0';
    }
    else if(flag_1 == 1)
    {
        t = pHead;
        while(t)
        {
            if(strcmp(recv_pack->data.recv_name, t->name) == 0 && (t->statu_s != OFFLINE))
            {
                ch[0] = '1';
                fd = t->fd;
                strcpy(recv_pack->file.mes, recv_pack->data.mes);
                break;
            }
            else if(strcmp(recv_pack->data.recv_name, t->name) == 0 && (t->statu_s == OFFLINE))
            {
                memcpy(&Mex_Box[sign++], recv_pack, sizeof(PACK));
                break;
            }
            t = t->next;
        }
    }
    send_mes(fd, flag, recv_pack, ch);
}

void out_grp(PACK *recv_pack)
{
    char need[1000];

    int flag = OUT_GRP;
    char ch[5];
    int fd = recv_pack->data.send_fd;

    Relation *q = pStart;
    int flag_1 = 0;
    while(q)
    {
        if(strcmp(q->name2, recv_pack->data.mes) == 0)
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }

    if(flag_1 == 0)
        ch[0] = '0';
    else
    {
        ch[0] = '1';
        Delete_R(q);

        memset(need, 0, strlen(need));
        sprintf(need, "delete from relation where user='%s' and oth_user='%s'", recv_pack->data.send_name, recv_pack->data.mes);
        mysql_real_query(&mysql, need, strlen(need));
    }
    send_mes(fd, flag, recv_pack, ch);
}

void del_grp(PACK *recv_pack)
{
    char need[1000];

    int flag = DEL_GRP;
    char ch[5];
    int fd = recv_pack->data.send_fd;

    Relation *q = pStart;
    int flag_1 = 0;
    int flag_2 = 0;
    while(q)
    {
        if(strcmp(q->name2, recv_pack->data.mes) == 0)
        {
            flag_2 = 1;
            break;
        }
        q = q->next;
    }

    q = pStart;
    while(q)
    {
        if(strcmp(q->name1, recv_pack->data.send_name) == 0 && strcmp(q->name2, recv_pack->data.mes) == 0 && (q->statu_s == GRP_OWN))
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }

    if(flag_2 == 0)
        ch[0] = '0';
    else if(flag_1 == 1 && flag_2 == 1)
    {
        ch[0] = '1';
        q = pStart;
        while(q)
        {
            if(strcmp(q->name2, recv_pack->data.mes) == 0)
                Delete_R(q);
            q = q->next;
        }
        memset(need, 0, strlen(need));
        sprintf(need, "delete from relation where oth_user='%s'", recv_pack->data.mes);
        mysql_real_query(&mysql, need, strlen(need));
    }
    else if(flag_1 == 0 && flag_2 == 1)
        ch[0] = '2';
    send_mes(fd, flag, recv_pack, ch);
}

void set_grp_adm(PACK *recv_pack)
{
    char need[1000];

    int flag = SET_GRP_ADM;
    char ch[5];
    int fd = recv_pack->data.send_fd;
    int fd2;
    User *t = pHead;
    Relation *q = pStart;
    int flag_3 = 0;
    int flag_2 = 0;
    int flag_1 = 0;
    while(q)
    {
        if(strcmp(q->name2, recv_pack->data.recv_name) == 0)
        {
            flag_2 = 1;
            break;
        }
        q = q->next;
    }

    q = pStart;
    while(q)
    {
        if(strcmp(q->name2, recv_pack->data.recv_name) == 0 && strcmp(q->name1, recv_pack->data.mes) == 0)
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }

    q = pStart;
    while(q)
    {
        if(strcmp(q->name1, recv_pack->data.send_name) == 0 && strcmp(q->name2, recv_pack->data.recv_name) == 0 && q->statu_s == GRP_OWN)
        {
            flag_3 = 1;
            break;
        }
        q = q->next;
    }

    if(flag_3 == 1 && flag_2 == 1 && flag_1 == 1)
    {
        ch[0] = '1';
        q = pStart;
        while(q)
        {
            if(strcmp(q->name1, recv_pack->data.mes) == 0 && strcmp(q->name2, recv_pack->data.recv_name) == 0)
            {
                q->statu_s = GRP_ADM;
                break;
            }
            q = q->next;
        }
        while(t)
        {
            if(strcmp(t->name, recv_pack->data.mes) == 0 && (t->statu_s != OFFLINE))
            {
                fd2 = t->fd;
                send_mes(fd2, flag, recv_pack, "6");
            }
            else if(strcmp(t->name, recv_pack->data.mes) == 0 && (t->statu_s == OFFLINE))
            {
                memcpy(&Mex_Box[sign++], recv_pack, sizeof(PACK));
            }
            t = t->next;
        }
        memset(need, 0, strlen(need));
        sprintf(need, "update relation set status=%d where user='%s' and oth_user='%s'", GRP_ADM, recv_pack->data.mes, recv_pack->data.recv_name);
        mysql_real_query(&mysql, need, strlen(need));
    }
    else if(flag_3 == 0 && flag_2 == 1 && flag_1 == 1)
        ch[0] = '2';
    else if(flag_1 == 0)
        ch[0] = '3';
    else if(flag_2 == 0)
        ch[0] = '0';
    send_mes(fd, flag, recv_pack, ch);
}

void kick_grp(PACK *recv_pack)
{
    char need[1000];

    int flag = KICK_GRP;
    char ch[5];
    int fd = recv_pack->data.send_fd;
    int fd2;
    User *t = pHead;
    Relation *q = pStart;
    int flag_3 = 0;
    int flag_1 = 0;
    int flag_2 = 0;
    int flag_4 = 0;
    while(q)
    {
        if(strcmp(q->name2, recv_pack->data.recv_name) == 0)
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }

    q = pStart;
    while(q)
    {
        if(strcmp(q->name2, recv_pack->data.recv_name) == 0 && strcmp(q->name1, recv_pack->data.mes) == 0)
        {
            flag_2 = 1;
            break;
        }
        q = q->next;
    }

    q = pStart;
    while(q)
    {
        if(strcmp(q->name1, recv_pack->data.send_name) == 0 && strcmp(q->name2, recv_pack->data.recv_name) == 0 && (q->statu_s == GRP_OWN || q->statu_s == GRP_ADM))
        {
            flag_3 = 1;
            break;
        }
        q = q->next;
    }
    
    q = pStart;
    while(q)
    {
        if(strcmp(q->name1, recv_pack->data.mes) == 0 && (q->statu_s == GRP))
        {
            flag_4 = 1;
            break;
        }
        q = q->next;
    }

    if(flag_3 == 1 && flag_1 == 1 && flag_2 == 1 && flag_4 == 1)
    {
        ch[0] = '1';
        Delete_R(q);
        while(t)
        {
            if(strcmp(t->name, recv_pack->data.mes) == 0 && (t->statu_s != OFFLINE))
            {
                fd2 = t->fd;
                send_mes(fd2, flag, recv_pack, "6");
            }
            else if(strcmp(t->name, recv_pack->data.mes) == 0 && (t->statu_s == OFFLINE))
            {
                memcpy(&Mex_Box[sign++], recv_pack, sizeof(PACK));
            }
            t = t->next;
        }
        memset(need, 0, strlen(need));
        sprintf(need, "delete from relation where user='%s' and oth_user='%s'", recv_pack->data.mes, recv_pack->data.recv_name);
        mysql_real_query(&mysql, need, strlen(need));
    }
    else if(flag_3 == 0 && flag_1 == 1 && flag_2 == 1 && flag_4 == 1)
        ch[0] = '2';
    else if(flag_4 == 0)
        ch[0] = '4';
    else if(flag_2 == 0)
        ch[0] = '3';
    else if(flag_1 == 0)
        ch[0] = '0';
    send_mes(fd, flag, recv_pack, ch);
}

void check_grp(PACK *recv_pack)
{
    int flag = CHECK_GRP;
    int fd = recv_pack->data.send_fd;
    Relation *q = pStart;
    int i = 0;

    while(q)
    {
        if(strcmp(q->name1, recv_pack->data.send_name) == 0 && (q->statu_s == GRP || q->statu_s == GRP_OWN || q->statu_s == GRP_ADM))
        {
            strcpy(recv_pack->grp_info.groups[i], q->name2);
            i++;
        }
        q = q->next;
    }

    recv_pack->grp_info.grp_num = i;

    send_mes(fd, flag, recv_pack, "");
}

void check_mem_grp(PACK *recv_pack)
{
    int flag = CHECK_MEM_GRP;
    int fd = recv_pack->data.send_fd;
    Relation *q = pStart;
    int i = 0;

    while(q)
    {
        if(strcmp(q->name2, recv_pack->data.mes) == 0 && (q->statu_s == GRP || q->statu_s == GRP_OWN || q->statu_s == GRP_ADM))
        {
            strcpy(recv_pack->fri_info.friends[i], q->name1);
            i++;
        }
        q = q->next;
    }
    recv_pack->fri_info.friends_num = i;

    send_mes(fd, flag, recv_pack, "");
}

void chat_many(PACK *recv_pack)
{
    int flag = CHAT_MANY;
    char ch[5];
    int fd = recv_pack->data.send_fd;
    char ss[MAX_CHAR];
    time_t now;
    char *str;
    
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char need[1000];
    int rows;
    int fields;
    PACK recv_t;
    recv_t.type = flag;
    RECORD_INFO rec_info[100];
    int i = 0,j = 0;

    User *t = pHead;
    Relation *q = pStart;
    int flag_1 = 0;

    Recordinfo *pNew = (Recordinfo *)malloc(sizeof(Recordinfo));

    if(strcmp(recv_pack->data.mes, "q") == 0)
    {
        while(t)
        {
            if(strcmp(t->name, recv_pack->data.send_name) == 0)
            {
                t->statu_s = ONLINE;
                free(pNew);
                pNew = NULL;
                return;
            }
            t = t->next;
        }
    }

    while(q)
    {
        if(strcmp(q->name2, recv_pack->data.recv_name) == 0 && (q->statu_s >= GRP))
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }
    if(flag_1 == 0)
    {
        ch[0] = '0';
        send_mes(fd, flag, recv_pack, ch);
        free(pNew);
        pNew = NULL;
        return;
    }
    else
    {
        if(strcmp(recv_pack->data.mes, "1") == 0)
        {
            memset(need, 0, strlen(need));
            sprintf(need, "select * from message where oth_user='%s'", recv_pack->data.recv_name);
            mysql_real_query(&mysql, need, strlen(need));
            res = mysql_store_result(&mysql);
            rows = mysql_num_rows(res);
            fields = mysql_num_fields(res);
            if(rows != 0)
            {
                while(row = mysql_fetch_row(res))
                {
                    if(rows <= 30)
                    {
                        strcpy(recv_pack->rec_info[i].name1, row[0]);
                        strcpy(recv_pack->rec_info[i].name2, row[1]);
                        strcpy(recv_pack->rec_info[i].message, row[2]);
                    }
                    else
                    {
                        if(rows - i <= 30)
                        {
                            strcpy(recv_pack->rec_info[j].name1, row[0]);
                            strcpy(recv_pack->rec_info[j].name2, row[1]);
                            strcpy(recv_pack->rec_info[j].message, row[2]);
                            j++;
                        }
                    }
                    i++;
                }
            }
            if(rows <= 30)
                recv_pack->rec_info[i].message[0] = '0';
            else
                recv_pack->rec_info[j].message[0] = '0';

            send_mes(fd, flag, recv_pack, "6");
            free(pNew);
            pNew = NULL;

            t = pHead;
            while(t)
            {
                if(strcmp(t->name, recv_pack->data.send_name) == 0)
                {
                    t->statu_s = MANY_CHAT;
                    strcpy(t->chat, recv_pack->data.recv_name);
                    break;
                }
                t = t->next;
            }
            q = pStart;
            while(q)
            {
                if(strcmp(q->name1, recv_pack->data.send_name) != 0 && strcmp(q->name2, recv_pack->data.recv_name) == 0 && (q->statu_s >= GRP))
                {
                    t = pHead;
                    while(t)
                    {
                        if(strcmp(q->name1, t->name) == 0 && (t->statu_s != OFFLINE))
                        {
                            ch[0] = '1';
                            fd = t->fd;
                            send_mes(fd, flag, recv_pack, ch);
                            break;
                        }
                        else if(strcmp(q->name1, t->name) == 0 && (t->statu_s == OFFLINE))
                        {
                            strcpy(recv_t.data.send_name, t->name);
                            strcpy(recv_t.data.recv_name, recv_pack->data.recv_name);
                            memcpy(&Mex_Box[sign++], &recv_t, sizeof(PACK));      
                            break;
                        }
                        t = t->next;
                    }
                }
                q = q->next;
            }
        }
        else
        {
            strcpy(pNew->name1, recv_pack->data.send_name);
            strcpy(pNew->name2, recv_pack->data.recv_name);
            strcpy(pNew->message, recv_pack->data.mes);
            Insert_RC(pNew);
            memset(need, 0, strlen(need));
            sprintf(need, "insert into message values('%s', '%s', '%s')", recv_pack->data.send_name, recv_pack->data.recv_name, recv_pack->data.mes);
            mysql_real_query(&mysql, need, strlen(need));

            q = pStart;
            while(q)
            {
                if(strcmp(q->name2, recv_pack->data.recv_name) == 0 && (q->statu_s >= GRP))
                {
                    t = pHead;
                    while(t)
                    {
                        if(strcmp(q->name1, t->name) == 0 && strcmp(t->chat, recv_pack->data.recv_name) == 0 && (t->statu_s == MANY_CHAT))
                        {
                            fd = t->fd;
                            bzero(ss, MAX_CHAR);
                            strcpy(ss,recv_pack->data.recv_name);
                            strcpy(recv_pack->data.recv_name, recv_pack->data.send_name);
                            time(&now);
                            str = ctime(&now);
                            str[strlen(str) - 1] = '\0';
                            memcpy(recv_pack->data.send_name, str, strlen(str));
                            send_mes(fd, flag, recv_pack, recv_pack->data.mes);
                            strcpy(recv_pack->data.send_name, ss);
                            bzero(ss, MAX_CHAR);
                            strcpy(ss,recv_pack->data.recv_name);
                            strcpy(recv_pack->data.recv_name, recv_pack->data.send_name);
                            strcpy(recv_pack->data.send_name, ss);
                            break;
                        }
                        t = t->next;
                    }
                }
                q = q->next;
            }
        }
    }
}


void check_mes_grp(PACK *recv_pack)
{
    int i = 0;
    int flag = CHECK_MES_GRP;
    char ch[5];
    int fd = recv_pack->data.send_fd;
    Relation *q = pStart;
    Recordinfo *p = pRec;
    int flag_1 = 0;
    while(q)
    {
        if(strcmp(q->name1, recv_pack->data.send_name) == 0 && strcmp(q->name2, recv_pack->data.mes) == 0 && (q->statu_s >= GRP)) 
        {
            flag_1 = 1;
            break;
        }
        q = q->next;
    }
    if(flag_1 == 0)
        ch[0] = '0';
    else
    {
        ch[0] = '1';
        while(p)
        {
            if((strcmp(p->name2, recv_pack->data.mes) == 0)) 
            {
                strcpy(recv_pack->rec_info[i].name1, p->name1);
                strcpy(recv_pack->rec_info[i].name2, p->name2);
                strcpy(recv_pack->rec_info[i].message, p->message);
                i++;
                if(i > 50)
                    break;
            }
            p = p->next;
        }
    }
    recv_pack->rec_info[i].message[0] = '0';
    
    send_mes(fd, flag, recv_pack, ch);
}

void recv_file(PACK *recv_pack)
{
    int flag = RECV_FILE;
    int fd = recv_pack->data.send_fd;
    int length = 0;
    int i = 0;
    char mes[MAX_CHAR * 3 + 1];
    char *name;
    bzero(mes, MAX_CHAR * 3 + 1);
    int fp;
    User *t = pHead;
    int flag_1 = 0;
    if(strcmp(recv_pack->data.mes,"8888") == 0)
    {
        while(t)
        {
            if(strcmp(t->name, recv_pack->data.recv_name) == 0)
            {
                flag_1 = 1;
                break;
            }
            t = t->next;
        }
        if(flag_1 == 1)
        {
            file.file_name[file.sign_file][0] = '_';
            for(i = 0; i < strlen(recv_pack->data.send_name); i++)
            {
                if(recv_pack->data.send_name[i] == '/')
                {
                    name = strrchr(recv_pack->data.send_name, '/');
                    name++;
                    strcat(file.file_name[file.sign_file],name);
                    break;
                }
            }
            if(i == strlen(recv_pack->data.send_name))
                strcat(file.file_name[file.sign_file],recv_pack->data.send_name);

            strcpy(file.file_send_name[file.sign_file], recv_pack->data.recv_name);
            fp = creat(file.file_name[file.sign_file], S_IRWXU);
            file.sign_file++;
            close(fp);
            send_mes(fd, flag, recv_pack, "1");
        }
        else 
            send_mes(fd, flag, recv_pack, "0");
    }
    else if(strcmp(recv_pack->data.mes, "ok") == 0)
    {
        while(t)
        {
            if(strcmp(t->name, recv_pack->data.recv_name) == 0 && (t->statu_s != OFFLINE))
            {
                flag_1 = 1;
                break;
            }
            t = t->next;
        }
        if(flag_1 == 1)
            send_file(recv_pack);
        else if(flag_1 == 0)
            memcpy(&Mex_Box[sign++], recv_pack, sizeof(PACK));    
    }
    else
    {
        for(i = 0; i < file.sign_file; i++)
        {
            if(strcmp(recv_pack->data.recv_name, file.file_send_name[i]) == 0)
            {
                fp = open(file.file_name[i], O_WRONLY | O_APPEND);
                break;
            }
        }
        if(write(fp, recv_pack->file.mes, recv_pack->file.size) < 0)
            my_err("write", __LINE__);
        close(fp);
    }
}

void send_file(PACK *recv_pack)
{
    int flag = SEND_FILE;
    int fd = recv_pack->data.send_fd;
    int fd2;
    int fp;
    int length = 0;
    PACK send_file;
    send_file.type = flag;

    char ss[MAX_CHAR];
    User *t = pHead;
    int flag_2 = 0;
    int i = 0;
    pthread_mutex_lock(&mutex);
    while(t)
    {
        if(strcmp(t->name, recv_pack->data.recv_name) == 0)
        {
            fd2 = t->fd;
            break;
        }
        t = t->next;
    }

    if(strcmp(recv_pack->data.mes, "ok") == 0)
    {
        strcpy(ss,recv_pack->data.recv_name);
        strcpy(recv_pack->data.recv_name, recv_pack->data.send_name);
        strcpy(recv_pack->data.send_name, ss);
        send_mes(fd2, flag, recv_pack, "request");
    }
    else if(recv_pack->data.mes[0] == 'y')
    {
        for(i = 0; i < file.sign_file; i++)
            if(strcmp(file.file_send_name[i], recv_pack->data.send_name) == 0)
                break;
        send_mes(fd2, flag, recv_pack, "consent");
        strcpy(recv_pack->data.recv_name, file.file_name[i]);
        send_mes(fd, flag, recv_pack, "8888");

        strcpy(send_file.data.send_name, recv_pack->data.recv_name);
        strcpy(send_file.data.recv_name, recv_pack->data.send_name);
        fp = open(file.file_name[i], O_RDONLY);
        if(fp == -1)
            printf("file: %s not find\n", file.file_name[i]);
        while((length = read(fp, send_file.file.mes, MAX_FILE - 1)) > 0)
        {
            send_file.file.size = length;
            if(send(fd, &send_file, sizeof(PACK), 0) < 0)
                my_err("send",__LINE__);
            bzero(send_file.file.mes, MAX_FILE);
        }
        printf("发送成功!\n");
        send_mes(fd, flag, recv_pack, "end");
        send_mes(fd2, flag, recv_pack, "finish");
        remove(file.file_name[i]);
        file.file_send_name[i][0] = '\0';
        close(fp);
    }
    else if(recv_pack->data.mes[0] == 'n')
    {
        send_mes(fd2, flag, recv_pack, "no");
        for(i = 0; i < file.sign_file; i++)
            if(strcmp(file.file_send_name[i], recv_pack->data.send_name) == 0)
                break;
        remove(file.file_name[i]);
        file.file_send_name[i][0] = '\0';
    }
    pthread_mutex_unlock(&mutex);
}

void send_mes(int fd, int type, PACK *recv_pack, char *mes)
{
    PACK pack_send;
    char ss[MAX_CHAR];
    memcpy(&pack_send, recv_pack, sizeof(PACK));
    strcpy(ss,pack_send.data.recv_name);
    
    pack_send.type = type;
    strcpy(pack_send.data.recv_name, pack_send.data.send_name);
    strcpy(pack_send.data.send_name, ss);
    strcpy(pack_send.data.mes, mes);
    pack_send.data.recv_fd = pack_send.data.send_fd;
    pack_send.data.send_fd = fd;

    if(send(fd, &pack_send, sizeof(PACK), 0) < 0)
        my_err("send", __LINE__);
}

void send_pack(int fd, PACK *recv_pack, char *ch)
{
    PACK pack_send;
    memcpy(&pack_send, recv_pack, sizeof(PACK));
    strcpy(pack_send.data.recv_name, pack_send.data.send_name);
    strcpy(pack_send.data.send_name, "server");
    strcpy(pack_send.data.mes, ch);
    printf("%s\n", pack_send.data.mes);
    pack_send.data.recv_fd = pack_send.data.send_fd;
    pack_send.data.send_fd = fd;
    if(send(fd, &pack_send, sizeof(PACK), 0) < 0)
        my_err("send", __LINE__);
}

void DeleteLink()		
{
    User *q = pHead;
    if(pHead == NULL)
        return;
    while(pHead)
    {
        q = pHead->next;
        free(pHead);
        pHead = q;
    }
	pHead = NULL;
}

void DeleteLink_R()		
{
    Relation *q = pStart;
    if(pStart == NULL)
        return;
    while(pStart)
    {
        q = pStart->next;
        free(pStart);
        pStart = q;
    }
	pStart = NULL;
}

void DeleteLink_RC()
{
    Recordinfo *q = pRec;
    if(pRec == NULL)
        return;
    while(pRec)
    {
        q = pRec->next;
        free(pRec);
        pRec = q;
    }
	pRec = NULL;
}
