#include "common.hh"

size_t readOneNaluFromBuffer(const unsigned char *buffer, unsigned int nBufferSize, unsigned int offSet, NaluUnit &nalu) {
    unsigned int i = offSet;

    while (i < nBufferSize) {
        if (buffer[i++] == 0x00 && buffer[i++] == 0x00 && buffer[i++] == 0x00 && buffer[i++] == 0x01) {
            unsigned int pos = i;
            while (pos<nBufferSize) {
                if (buffer[pos++] == 0x00 && buffer[pos++] == 0x00 && buffer[pos++] == 0x00 && buffer[pos++] == 0x01) {
                    break;
                }
            }
            if (pos == nBufferSize) {
                nalu.size = pos - i;  
            }
            else {
                nalu.size = (pos - 4) - i;
            }
 
            nalu.type = buffer[i] & 0x1f;
            nalu.data =(unsigned char*) & buffer[i];
            return (nalu.size + i - offSet);
        }
    }
    return 0;
}
