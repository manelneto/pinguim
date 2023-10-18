// Application layer protocol implementation

#include "application_layer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>


#include "link_layer.h"

#define CONTROL_PACKET_START 0x02
#define CONTROL_PACKET_END 0x03
#define DATA_PACKET 0x01
#define MAX_DATA_SIZE 1000

int logaritmo2(int n) {
    int res = 0;
    while (n > 0) {
        n >>= 1;
        res++;
    }
    return res;
}

unsigned char *buildControlPackets(long int fileSize, const char *fileName, unsigned char controlField, int *packetSize) {

    int fileSizeLength = (int) (logaritmo2(fileSize) / 8.0) + 1 ; //bytes necessários para representar o tamanho do ficheiro -- +1 para arrendondar para cima

    int fileNameLength = strlen(fileName);
    *packetSize = 5 + fileSizeLength + fileNameLength; // 5 -> C + T1 + L1 + T2 + L2
    unsigned char *controlPacket = (unsigned char *) malloc(*packetSize);

    controlPacket[0] = controlField;
    controlPacket[1] = 0;  // File size parameter
    controlPacket[2] = fileSizeLength;  // File size length
    int index;
    for(index = 3; index < (fileSizeLength + 3); index++){
        unsigned leftmost = fileSize & 0xFF << (fileSizeLength - 1) * 8;
        leftmost >>= (fileSizeLength - 1) * 8;
        fileSize <<= 8;
        controlPacket[index] = leftmost;
    }
    controlPacket[++index] = 1;  // File name parameter
    controlPacket[++index] = fileNameLength;  // File name 
    memcpy(controlPacket + index, fileName, fileNameLength);

    for (unsigned i = 0; i < *packetSize; i++) printf("0x%x\n", controlPacket[i]);

    return controlPacket;
}

unsigned char *buildDataPackets(int dataSize, unsigned char *data, int *packetSize) {

    unsigned char *dataPacket = (unsigned char *) malloc(*packetSize);

    dataPacket[0] = 1;
    dataPacket[1] = dataSize / 256;  
    dataPacket[2] = dataSize % 256;
    memcpy(dataPacket + 3, data, dataSize);

    return dataPacket;
}

void buildDataForPackets(int dataSize,  unsigned char *data, unsigned char* fileContent){
    data = (unsigned char *) malloc(dataSize);
    memcpy(data, fileContent, dataSize);
}


void applicationLayer(const char *serialPort, const char *role, int baudRate, int nTries, int timeout, const char *filename) {
    // TODO
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort, serialPort);
    if (strcmp(role, "tx") == 0)  // role == "tx"
        connectionParameters.role = LlTx;
    else if (strcmp(role, "rx") == 0)  // role == "rx"
        connectionParameters.role = LlRx;
    else {  
        printf("Invalid role\n");
        exit(-1);
    }

    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    if(llopen(connectionParameters) != 0){
        printf("Error opening connection\n");
        exit(-1);
    }

    if (connectionParameters.role == LlTx) {
        
        FILE *file = fopen(filename, "rb");
        if (file == NULL) {
            printf("Error opening file\n");
            exit(-1);
        }

        long int fileSize;
        struct stat st;
        if (stat(filename, &st) == 0) {
            fileSize = st.st_size;
            printf("O tamanho do ficheiro é %ld bytes\n", fileSize);
        }
        else {
            perror("Erro ao obter o tamanho do ficheiro");
            exit(-1);
        }

        // Send first control packet
        int controlStartPacketSize;
        unsigned char *controlStartPacket = buildControlPackets(fileSize, filename, CONTROL_PACKET_START, &controlStartPacketSize);
        if (llwrite(controlStartPacket, controlStartPacketSize) != 0) { 
            perror("Error sending control packet");
            exit(-1);
        }

        unsigned char * fileContent = (unsigned char *) malloc(fileSize * sizeof(unsigned char));
        fread(fileContent, sizeof(unsigned char), fileSize, file);

        int completePackets = fileSize / MAX_DATA_SIZE;
        int incompletePacketSize = fileSize - (MAX_DATA_SIZE * completePackets);

        for(int i = 0; i < completePackets; i++){ 

            unsigned char data;
            buildDataForPackets(MAX_DATA_SIZE, &data, fileContent);
    
            int dataPacketSize;
            unsigned char *dataPacket = buildDataPackets(MAX_DATA_SIZE, &data, &dataPacketSize);

            if (llwrite(dataPacket, dataPacketSize)) {
                perror("Error sending data packet");
                exit(-1);
            }

            fileContent += MAX_DATA_SIZE;
        }

        if(incompletePacketSize != 0){
            unsigned char data;
            buildDataForPackets(incompletePacketSize, &data, fileContent);
    
            int dataPacketSize;
            unsigned char *dataPacket = buildDataPackets(incompletePacketSize, &data, &dataPacketSize);

            if (llwrite(dataPacket, dataPacketSize)) {
                perror("Error sending data packet");
                exit(-1);
            }
        }

        // Send end control packet
        int controlEndPacketSize;
        unsigned char *controlEndPacket = buildControlPackets(fileSize, filename, CONTROL_PACKET_END, &controlEndPacketSize);
        if (llwrite(controlEndPacket, controlEndPacketSize) != 0) { 
            perror("Error sending control packet");
            exit(-1);
        }

    } 
    else if (connectionParameters.role == LlRx) {
        /*unsigned char *buf1 = (unsigned char *)malloc(5);
        while (llread(buf1));*/

        while(){

        }


       
    }
    
    if(llclose(0) != 0){ // <-- o fabio meter isto dentro do tx
        printf("Error closing connection\n");
        exit(-1);
    }
}



