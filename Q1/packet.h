#define BUFSIZE 100 
#define PDR 50
#define PORT_SERVER 8888
#define PACKET_SIZE 100
#define TIME_OUT 1
typedef struct

{
    int size; //data size in bytes
    char data[PACKET_SIZE];
    int seq;       //sequence no
    int flag_last; //if last data
    int flag_ack;  // 0 for data 1 for flag
    int flag_chan; // 0 for channel 0
} PACKET;

