#include <stdio.h>
#include <inttypes.h>

#include "ftd2xx.h"
#include "srix4k.h"

FT_HANDLE ftHandle;

int main(int argc, char *argv[])
{
    unsigned char i;
    uint32_t block;
    uint64_t UID;

    ///=================================
    /// INIZIALIZZAZIONE
    ///=================================
    if (!PN532_Init(&ftHandle))
    {
        printf("PN532 Init fail\n");
        exit(0);
    }
    if (!PN532_SAMConfiguration(ftHandle))
    {
        printf("PN532 SAMConfiguration command fail\n");
        exit(0);
    }
    if (!PN532_GetFirmwareVersion(ftHandle))
    {
        printf("PN532 GetFirmwareVersion command fail\n");
        exit(0);
    }
    if (!PN532_RFconfiguration(ftHandle))
    {
        printf("PN532 RFconfiguration command fail\n");
        exit(0);
    }
    if (!PN532_InlistPassiveTarget(ftHandle))
    {
        printf("PN532 InlistPassiveTarget command fail\n");
        exit(0);
    }
    if (!PN532_Initiate_Select(ftHandle))
    {
        printf("PN532 Initiate_Select command fail\n");
        exit(0);
    }

    ///=================================
    /// LETTURA
    ///=================================

    ///Lettura blocchi da 0x00 a 0x7F
    for (i=0;i<128;i++)
    {
        printf("\n%02X : ",i);
        if (!PN532_ReadBlock(ftHandle,i,&block))
        {
            printf("PN532 ReadBlock command fail\n");
            exit(0);
        }
        printf("%08X",block);
    }
    printf("\n%02X : ",0xFF);
    if (!PN532_ReadBlock(ftHandle,0xFF,&block))
    {
        printf("PN532 ReadBlock command fail\n");
        exit(0);
    }
    printf("%08X\n",block);

    ///=================================
    /// SCRITTURA
    ///=================================
    //Scrittura blocco 6E
    //block = 0xFFFFFFFF;
    //PN532_WriteBlock(0x6E, &block);

    if (!PN532_GetUID(ftHandle,&UID))
    {
        printf("PN532 GetUID command fail\n");
        exit(0);
    }
    printf("UID = %" PRIx64 "\n", UID);

    ///=================================
    /// FINE
    ///=================================

    //Chiudi PN532
    if (!PN532_Close(ftHandle))
    {
        printf("PN532 Close fail\n");
        exit(0);
    }

    return 0;
}
