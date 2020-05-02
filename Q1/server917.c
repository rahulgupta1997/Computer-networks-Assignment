#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "packet.h"


double randgen(int bar)
{
    return (rand() / ((double)RAND_MAX)) * bar;
}

int offsetReqd = 0;
int numberOfPkts = 0;
void die(char *s)
{
    perror(s);
    exit(1);
}
int main(void)
{

    //creation of file
    FILE *fp;
    fp = fopen("destination.txt", "a+");
    fseek(fp, 0, SEEK_END);
    int offset = ftell(fp);
    //client 0 client 1 master
    int socket_set[3]; //0 master
    int addrlen;

    //creation of socket for server
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(8888);

    PACKET arr[BUFSIZE];
    PACKET rcv_pkt;
    PACKET ack_pkt;
    int maxSd = 0;
    fd_set readfds;

    //master socket creation complete
    socket_set[2] = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_set[2] < 0)
    {
        perror("Unable to create Server");
        exit(0);
    }
    if (bind(socket_set[2], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Unable to Bind");
        exit(0);
    }
    if (listen(socket_set[2], 2) == -1)
    {
        perror("Unable to Listen");
        exit(0);
    }

    int signal = 0;
    addrlen = sizeof(serv_addr);
    int flag_l;
    while (signal != 1)
    {
        flag_l = 1;
        //  printf("\nINSIDE WHILE\n");
        FD_ZERO(&readfds);
        FD_SET(socket_set[2], &readfds);
        maxSd = socket_set[2];
        for (int e = 0; e < 2; e++)
        {
            FD_SET(socket_set[e], &readfds);
            if (socket_set[e] > maxSd)
            {
                maxSd = socket_set[e];
            }
        }
        int wait = select(maxSd + 1, &readfds, NULL, NULL, NULL);
        if (wait < 0)
        {
            printf("ERROR");
        }
        if (FD_ISSET(socket_set[2], &readfds))
        {
            if ((socket_set[0] = accept(socket_set[2],
                                        (struct sockaddr *)&serv_addr, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            if ((socket_set[1] = accept(socket_set[2],
                                        (struct sockaddr *)&serv_addr, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
        }
        // there is some readable socket descriptor
        if (FD_ISSET(socket_set[0], &readfds))
        {
            int temp = -1;
            if (signal == 0)
                temp = recv(socket_set[0], &rcv_pkt, sizeof(rcv_pkt), 0);
            if (temp == -1)
            {
                printf("ERROR in reciving");
            }
            double prob = randgen(100);
            if (prob < PDR)
            {
                flag_l = 0;
            }
            if (flag_l == 1)
            {
                printf("RCVD : Seq no %d of size %d Bytes from channel %d\n", rcv_pkt.seq, rcv_pkt.size, rcv_pkt.flag_chan);
                if (rcv_pkt.seq == offsetReqd)
                {
                    if (rcv_pkt.flag_last == 1)
                    {
                        signal = 1;
                    }
                    int numElements = fwrite(rcv_pkt.data, sizeof(char), rcv_pkt.size, fp);
                    if (numElements != rcv_pkt.size)
                    {
                        printf("ERROR n");
                    }
                    offsetReqd += rcv_pkt.size;
                    int iterator = 0;
                    for (int i = 0; i < numberOfPkts; i++)
                    {
                        if (arr[i].seq == offsetReqd)
                        {
                            if (arr[i].flag_last == 1)
                            {
                                signal = 1;
                            }
                            iterator++;
                            fwrite(arr[i].data, sizeof(char), arr[i].size, fp);
                            offsetReqd += arr[i].size;
                        }
                        else
                        {
                            break;
                        }
                    }
                    for (int i = iterator; i < numberOfPkts; i++)
                    {
                        arr[i - iterator] = arr[i];
                    }
                    numberOfPkts -= iterator;
                }
                else
                {
                    if (numberOfPkts == 0)
                    {
                        arr[0] = rcv_pkt;
                        numberOfPkts += 1;
                    }
                    else
                    {
                        int insert = 0;
                        for (int i = numberOfPkts - 1; i >= 0; i--)
                        {
                            if (rcv_pkt.seq > arr[i].seq)
                            {
                                insert = 1;
                                arr[i + 1] = rcv_pkt;
                                break;
                            }
                            else
                            {
                                arr[i + 1] = arr[i];
                            }
                        }
                        if (insert == 0)
                        {
                            arr[0] = rcv_pkt;
                        }
                        numberOfPkts += 1;
                    }
                }
                ack_pkt.seq = rcv_pkt.seq;
                ack_pkt.flag_ack = 1;
                ack_pkt.flag_chan = rcv_pkt.flag_chan;
                send(socket_set[0], &ack_pkt, sizeof(ack_pkt), 0);
                printf("Send ACK : for packet with Seq no %d from channel %d\n", ack_pkt.seq, ack_pkt.flag_chan);
            }
        }
        if (FD_ISSET(socket_set[1], &readfds))
        {
            // printf("PACKET RECEIVED FROM CHANNEL 2\n");
            int temp = recv(socket_set[1], &rcv_pkt, sizeof(PACKET), 0);
            if (temp == -1)
            {
                printf("Problem in client_Socket 2\n");
            }
            double prob = randgen(100);
            if (prob < PDR)
            {
                flag_l = 0;
            }
            if (flag_l == 1)
            {
                printf("RCVD : Seq no %d of size %d Bytes from channel %d\n", rcv_pkt.seq, rcv_pkt.size, rcv_pkt.flag_chan);
                if (rcv_pkt.seq == offsetReqd)
                {
                    // printf("\nPACKET RECEIVED FROM SECOND CHANNEL WRITING INTO FILE\n");
                    if (rcv_pkt.flag_last == 1)
                    {
                        signal = 1;
                    }
                    fwrite(rcv_pkt.data, sizeof(char), rcv_pkt.size, fp);
                    // check if there are other pkts
                    offsetReqd += rcv_pkt.size;
                    int iterator = 0;
                    for (int i = 0; i < numberOfPkts; i++)
                    {
                        if (arr[i].seq == offsetReqd)
                        {
                            if (arr[i].flag_last == 1)
                            {
                                signal = 1;
                            }
                            fwrite(arr[i].data, sizeof(char), arr[i].size, fp);
                            offsetReqd += arr[i].size;
                            iterator++;
                        }
                        else
                        {
                            break;
                        }
                    }
                    for (int i = iterator; i < numberOfPkts; i++)
                    {
                        arr[i - iterator] = arr[i];
                    }
                    numberOfPkts -= iterator;
                }
                else
                {
                    if (numberOfPkts == 0)
                    {
                        arr[0] = rcv_pkt;
                        numberOfPkts += 1;
                    }
                    else
                    {
                        int insert = 0;
                        for (int i = numberOfPkts - 1; i >= 0; i--)
                        {
                            if (rcv_pkt.seq > arr[i].seq)
                            {
                                insert = 1;
                                arr[i + 1] = rcv_pkt;
                                break;
                            }
                            else
                            {
                                arr[i + 1] = arr[i];
                            }
                        }
                        if (insert == 0)
                        {
                            arr[0] = rcv_pkt;
                        }
                        numberOfPkts += 1;
                    }
                }
                ack_pkt.seq = rcv_pkt.seq;
                ack_pkt.flag_ack = 1;
                ack_pkt.flag_chan = rcv_pkt.flag_chan;
                send(socket_set[1], &ack_pkt, sizeof(PACKET), 0);
                printf("Send ACK : for packet with Seq no %d from channel %d\n", ack_pkt.seq, ack_pkt.flag_chan);
            }
        }
    }
    fclose(fp);
    return 0;
}
