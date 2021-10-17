#ifndef _PACKET_H
#define _PACKET_H

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};


void stringToPacket(char* src, struct Packet* dest){
    int start = 0, end = 0;
    char* buf;

    // Extract total_fragment number from first Position
    while(src[end] != ':') end++;
    buf = malloc((end - start) * sizeof(char) + 1);
    memcpy(buf, &src[start], end - start);
    dest->total_frag = atoi(buf);
    end++;
    start = end;
    free(buf);

    // Extract frag_no number from second Position
    while(src[end] != ':') end++;
    buf = malloc((end - start) * sizeof(char) + 1);
    memcpy(buf, &src[start], end - start);
    dest->frag_no = atoi(buf);
    end++;
    start = end;
    free(buf);

    // Extract size from third Position
    while(src[end] != ':') end++;
    buf = malloc((end - start) * sizeof(char) + 1);
    memcpy(buf, &src[start], end - start);
    dest->size = atoi(buf);
    end++;
    start = end;
    free(buf);

    // Extract file name from fourth Position
    while(src[end] != ':') end++;
    dest->filename = malloc((end - start) * sizeof(char) + 1);
    memcpy(dest->filename, &src[start], end - start);
    end++;
    start = end;

    // Lastly, Extract file data from Position left
    memcpy(dest->filedata, &src[start], dest->size);
}

#endif
