#ifndef _PACKET_H
#define _PACKET_H

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

#endif