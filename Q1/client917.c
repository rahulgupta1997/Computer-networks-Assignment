//Rahul Gupta
//2016B2A70917P

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "packet.h"



int seq_no;
int packets_recvd = 0;
int total = 0;
int retrans(clock_t abc, int socketss, PACKET p)
{
    float secs = (clock() - abc) / CLOCKS_PER_SEC;
    if (secs > TIME_OUT)
    {
        send(socketss, &p, sizeof(p), 0);
        printf("Send   : Seq no %d of size %d Bytes from channel  %d\n", p.seq, p.size, p.flag_chan);
    }
}
PACKET packetMaker(FILE *fp, PACKET new_packet, int ch)
{

    int rd = fread(new_packet.data, sizeof(char), PACKET_SIZE, fp);
    // printf("%s \n", new_packet.data);
    new_packet.flag_ack = 0;
    new_packet.flag_chan = ch;
    new_packet.size = rd;
    seq_no = ftell(fp);
    new_packet.seq = seq_no;
    if (rd < PACKET_SIZE || fp == EOF)
    {
        new_packet.flag_last = 1;
    }
    else
    {
        new_packet.flag_last = 0;
    }
    return new_packet;
}
int helper(int a, int b)
{
    if (a > 0 && b > 0)
    {
        return 0;
    }
    else if (b > 0)
    {
        return 1;
    }
    else if (a > 0)
    {
        return 2;
    }
    else
    {
        return 3;
    }
}
int main()
{
    //variable declaration
    struct sockaddr_in sAd;
    FILE *fp;
    int client_sock_1, client_sock_2;
    int c1, c2;
    clock_t clock_1, clock_2;
    PACKET temp[2];
    PACKET rec_v[2];
    PACKET new_p;

    memset(&sAd, 0, sizeof(sAd));
    //init client socket
    client_sock_1 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock_1 < 0)
    {
        perror("Error:-Unable to open socket");
        exit(0);
    }
    client_sock_2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock_2 < 0)
    {
        perror("Error:-Unable to open socket");
        exit(0);
    }
    //server adress
    sAd.sin_family = AF_INET;
    sAd.sin_port = htons(PORT_SERVER);
    sAd.sin_addr.s_addr = inet_addr("127.0.0.1");
    //connecting o server

    fcntl(client_sock_1, F_SETFL, O_NONBLOCK);
    fcntl(client_sock_2, F_SETFL, O_NONBLOCK);
    c1 = connect(client_sock_1, (struct sockaddr *)&sAd, sizeof(sAd));
    c2 = connect(client_sock_2, (struct sockaddr *)&sAd, sizeof(sAd));

    char x[PACKET_SIZE];
    FILE *fp1 = fopen("input.txt", "r");
    while (1)
    {
        if (fread(x, 1, PACKET_SIZE, fp1) > 0)
            total++;
        else
        {
            break;
        }
    }
    fclose(fp1);
    fp = fopen("input.txt", "r");
    if (fp == NULL)
    {
        perror("Error:-opening file");
        //exit(0);
    }
    new_p = packetMaker(fp, new_p, 0);
    send(client_sock_1, &new_p, sizeof(new_p), 0);
    temp[0] = new_p;
    printf("Send   : Seq no %d of size %d Bytes from channel %d\n", new_p.seq, new_p.size, new_p.flag_chan);
    clock_1 = clock();

    new_p = packetMaker(fp, new_p, 1);
    temp[1] = new_p;
    send(client_sock_2, &new_p, sizeof(new_p), 0);
    printf("Send   : Seq no %d of size %d Bytes from channel %d\n", new_p.seq, new_p.size, new_p.flag_chan);
    clock_2 = clock();

    while (fp != NULL)
    {
        int a = recv(client_sock_1, &rec_v[0], sizeof(rec_v[0]), MSG_DONTWAIT);
        int b = recv(client_sock_2, &rec_v[1], sizeof(rec_v[1]), MSG_DONTWAIT);
        int state = helper(a, b);
        switch (state)
        {
        case 0:
            printf("RCVD  : for packet  with Seq no %d from channnel %d\n", rec_v[0].seq, rec_v[0].flag_chan);
            packets_recvd = packets_recvd + 2;
            new_p = packetMaker(fp, new_p, 0);
            temp[0] = new_p;
            send(client_sock_1, &new_p, sizeof(new_p), 0);
            printf("Send   : Seq no %d of size %d Bytes from channel %d\n", new_p.seq, new_p.size, new_p.flag_chan);
            clock_1 = clock();
            printf("RCVD  : for  packet with Seq no %d from channnel %d\n", rec_v[1].seq, rec_v[1].flag_chan);
            new_p = packetMaker(fp, new_p, 1);
            temp[1] = new_p;
            send(client_sock_2, &new_p, sizeof(new_p), 0);
            printf("Send   : Seq no %d of size %d Bytes from channel %d\n", new_p.seq, new_p.size, new_p.flag_chan);
            clock_2 = clock();

            break;
        case 1:
            printf("RCVD  : for packet  with Seq no %d from channnel %d\n", rec_v[1].seq, rec_v[1].flag_chan);
            new_p = packetMaker(fp, new_p, 1);
            temp[1] = new_p;
            send(client_sock_2, &new_p, sizeof(new_p), 0);
            printf("Send   : Seq no %d of size %d Bytes from channel %d\n", new_p.seq, new_p.size, new_p.flag_chan);

            packets_recvd = packets_recvd + 1;
            clock_2 = clock();

            retrans(clock_1, client_sock_1, temp[0]);
            break;
        case 2:
            printf("RCVD  : for packet with Seq no %d from channnel %d\n", rec_v[0].seq, rec_v[0].flag_chan);

            new_p = packetMaker(fp, new_p, 0);
            temp[0] = new_p;
            send(client_sock_1, &new_p, sizeof(new_p), 0);
            printf("Send   : Seq no %d of size %d Bytes from channel %d\n", new_p.seq, new_p.size, new_p.flag_chan);

            packets_recvd = packets_recvd + 1;
            clock_1 = clock();

            retrans(clock_2, client_sock_2, temp[1]);
            break;
        case 3:
            retrans(clock_1, client_sock_1, temp[0]);
            retrans(clock_2, client_sock_2, temp[1]);
            break;
        }
        if (packets_recvd > total)
            break;
    }
    close(client_sock_1);
    close(client_sock_2);
    exit(0);
}
