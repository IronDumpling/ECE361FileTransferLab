#ifndef _PACKET_H
#define _PACKET_H

#define DataSize 1000
#define PacketSize 1200

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[DataSize];
};

// Function 6. Packet to String
void packetToString(char destination[], struct packet* source){
    // Initialize
    memset(destination, '\0', PacketSize);

    unsigned long index;

    // Add total fragment
    sprintf(destination, "%d", source->total_frag);
    index = strlen(destination);
    destination[index++] = ':';

    // Add fragment number
    sprintf(destination + index, "%d", source->frag_no);
    index = strlen(destination);
    destination[index++] = ':';

    // Add Size
    sprintf(destination + index, "%d", source->size);
    index = strlen(destination);
    destination[index++] = ':';

    // Add filename
    sprintf(destination + index, "%s", source->filename);
    index += strlen(source->filename);
    destination[index++] = ':';

    // Add file data
    memcpy(destination + index, source->filedata, sizeof(char) * DataSize);
}

void stringToPacket(char* src, struct packet* dest) {
    int start = 0, end = 0;
    char *buf;

    // Extract total_fragment number from first Position
    while (src[end] != ':') end++;
    buf = malloc((end - start) * sizeof(char) + 1);
    memcpy(buf, &src[start], end - start);
    dest->total_frag = atoi(buf);
    end++;
    start = end;
    free(buf);

    // Extract frag_no number from second Position
    while (src[end] != ':') end++;
    buf = malloc((end - start) * sizeof(char) + 1);
    memcpy(buf, &src[start], end - start);
    dest->frag_no = atoi(buf);
    end++;
    start = end;
    free(buf);

    // Extract size from third Position
    while (src[end] != ':') end++;
    buf = malloc((end - start) * sizeof(char) + 1);
    memcpy(buf, &src[start], end - start);
    dest->size = atoi(buf);
    end++;
    start = end;
    free(buf);

    // Extract file name from fourth Position
    while (src[end] != ':') end++;
    dest->filename = malloc((end - start) * sizeof(char) + 1);
    memcpy(dest->filename, &src[start], end - start);
    end++;
    start = end;
}

#endif