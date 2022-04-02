#include <stdio.h>
#include <inttypes.h>
#include <time.h>

#include "ftd2xx.h"
#include "srix4k.h"

uint8_t PN532_Init(FT_HANDLE *ftHandle)
{
    FT_STATUS ftStatus = FT_OK;        ///Risposta del dispositivo FTDI
    DWORD driverVersion = 0;           ///Versione del driver FTDI

    ///Apertura dispositivo FTDI
    ftStatus = FT_Open(0,ftHandle);
    if (ftStatus != FT_OK)
    {
        LOG_PRINT(("Errore nell'apertura del dispositivo FTDI\n"));
        return 0;
    }

    ///Lettura della versione dei driver FTDI
    ftStatus = FT_GetDriverVersion(*ftHandle, &driverVersion);
	  if (ftStatus != FT_OK)
    {
        LOG_PRINT(("Errore. FT_GetDriverVersion = %d.\n",(int)ftStatus));
        return 0;
    }
    #ifdef DEBUG
    printf("Driver D2XX versione %08x\n", driverVersion);
    #endif // DEBUG

    ///Reset del dispositivo
    ftStatus = FT_ResetDevice(*ftHandle);
    if (ftStatus != FT_OK)
    {
        LOG_PRINT(("Errore.  FT_ResetDevice = %d.\n", (int)ftStatus));
        return 0;
    }

    ///Setting dei parametri della comunicazione seriale
    ftStatus = FT_SetDataCharacteristics(*ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
    if (ftStatus != FT_OK)
    {
        LOG_PRINT(("Errore.  FT_SetDataCharacteristics = %d.\n",(int)ftStatus));
        return 0;
    }

    ///Setting del baudrate
    ftStatus = FT_SetBaudRate(*ftHandle, 115200);
    if (ftStatus != FT_OK)
    {
        LOG_PRINT(("Errore.  FT_SetBaudRate = %d.\n", (int)ftStatus));
        return 0;
    }
    #ifdef DEBUG
    printf("Inizializzazione dispositivo avvenuta con successo\n");
    #endif // DEBUG

    return 1;
}

uint8_t PN532_WriteCMD(FT_HANDLE ftHandle, uint8_t *writeData, uint8_t w_len, uint8_t *readData, uint8_t r_len, uint8_t wakeup)
{
    int i = 0;
    int len = 0 ;
    FT_STATUS ftStatus = FT_OK;
    DWORD bytesWritten = 0;
    DWORD bytesReceived = 0;
    DWORD bytesRead = 0;
    unsigned char *outputData = NULL;
    unsigned char *readBuffer = NULL;
    unsigned char sum;
    unsigned char find;

    ///Calcolo la dimensione del vettore da inviare
    len = 7 + w_len;
    if (wakeup)
        len = len + 16;
    ///Allocate vector
    outputData = (unsigned char *)calloc(len+1, sizeof(unsigned char));
    if (wakeup)
    {
        sum = 0xD4;
        outputData[0]  = 0x55;
        outputData[1]  = 0x55;
        outputData[2]  = 0x00;
        outputData[3]  = 0x00;
        outputData[4]  = 0x00;
        outputData[5]  = 0x00;
        outputData[6]  = 0x00;
        outputData[7]  = 0x00;
        outputData[8]  = 0x00;
        outputData[9]  = 0x00;
        outputData[10] = 0x00;
        outputData[11] = 0x00;
        outputData[12] = 0x00;
        outputData[13] = 0x00;
        outputData[14] = 0x00;
        outputData[15] = 0x00;

        outputData[16] = 0x00;
        outputData[17] = 0x00;
        outputData[18] = 0xFF;
        outputData[19] = w_len + 1;
        outputData[20] = 256 - w_len - 1;
        outputData[21] = 0xD4;
        for (i=0;i<w_len;i++)
        {
          outputData[22+i] = writeData[i];
          sum = sum + writeData[i];
        }
        outputData[22+w_len] = 256 - sum;
        outputData[22+w_len+1] = 0x00;
        ftStatus = FT_Write(ftHandle, outputData, 22+w_len+1+1, &bytesWritten);

        #ifdef DEBUG
        printf("\nWrite buffer:\n");
        for (i=0;i<22+w_len+1+1;i++)
          printf("%02X ",(unsigned int)outputData[i]);
        #endif // DEBUG
    }
    else
    {
        sum = 0xD4;
        outputData[0]  = 0x00;
        outputData[1]  = 0x00;
        outputData[2]  = 0xFF;
        outputData[3]  = w_len + 1;
        outputData[4]  = 256 - w_len - 1;
        outputData[5]  = 0xD4;
        for (i=0;i<w_len;i++)
        {
          outputData[6+i] = writeData[i];
          sum = sum + writeData[i];
        }
        outputData[6+w_len] = 256 - sum;
        outputData[6+w_len+1] = 0x00;
        ftStatus = FT_Write(ftHandle, outputData, 6+w_len+1+1, &bytesWritten);

        #ifdef DEBUG
        printf("\nWrite buffer:\n");
        for (i=0;i<6+w_len+1+1;i++)
          printf("%02X ",(unsigned int)outputData[i]);
        #endif // DEBUG
    }
    if (ftStatus != FT_OK)
    {
    	  LOG_PRINT(("Errore durante FT_Write (error %d).\n", (int)ftStatus));
    	  free(outputData);
    	  return 0;
    }
    free(outputData);

    ///Leggo il numero di byte letti dal dispositivo FTDI
    ///Se non leggo il numero di byte attessi in un tempo < 500ms, segnalo un errore
    int msec = 0;
    clock_t before = clock();
    find = 0;
    do
    {
        ///Leggo lo stato del buffer di memoria dell'FTDI
        ftStatus = FT_GetQueueStatus(ftHandle, &bytesReceived);
        if (ftStatus != FT_OK)
        {
            LOG_PRINT(("Errore.  FT_GetQueueStatus = %d.\n",(int)ftStatus));
            return 0;
        }
        if (bytesReceived == (14 + r_len))
        {
            find = 1;
            break;
        }
        clock_t difference = clock() - before;
        msec = difference * 1000 / CLOCKS_PER_SEC;
        Sleep(5);
    } while ( msec < 500 );

    if (find==0)
    {
        LOG_PRINT(("Errore. Timeout raggiunto in attesa di una risposta dal dispositivo, bytesReceived = %i, rx_len = %i \n", bytesReceived,r_len));
        return 0;
    }

    ///Leggo lo stato del buffer di memoria dell'FTDI
    ftStatus = FT_GetQueueStatus(ftHandle, &bytesReceived);
    if (ftStatus != FT_OK)
    {
        LOG_PRINT(("Errore.  FT_GetQueueStatus = %d.\n",(int)ftStatus));
        return 0;
    }
    ///Creo un vettore che ha dimensioni pari al numero di byte letti dal dispositivo
    readBuffer = (unsigned char *)calloc(bytesReceived, sizeof(unsigned char));
    ///Leggo il contenuto del buffer FTDI
    ftStatus = FT_Read(ftHandle, readBuffer, bytesReceived, &bytesRead);

    #ifdef DEBUG
    printf("\nRead buffer:\n");
    for (i=0;i<bytesRead;i++)
      printf("%02X ",(unsigned int)readBuffer[i]);
    printf("\n");
    #endif // DEBUG

    ///Controllo che ci sia l'ACK frame
    if (!(readBuffer[0]==0x00 && readBuffer[1]==0x00 && readBuffer[2]==0xFF && readBuffer[3]==0x00 && readBuffer[4]==0xFF && readBuffer[5]==0x00))
    {
        LOG_PRINT(("Errore. ACK frame non ricevuto"));
        free(readBuffer);
        return 0;
    }
    ///Controllo il preambolo e lo start del frame
    if (!(readBuffer[6]==0x00 && readBuffer[7]==0x00 && readBuffer[8]==0xFF))
    {
        LOG_PRINT(("Errore. Preambolo e start of packed code non ricevuti correttamente"));
        free(readBuffer);
        #ifdef DEBUG
        for (i=6;i<9;i++)
          printf("%02X ",(unsigned int)readBuffer[i]);
        #endif // DEBUG
        return 0;
    }
    ///Controllo della lunghezza del pacchetto e del suo checksum
    if ((readBuffer[9]+readBuffer[10])!=256)
    {
        LOG_PRINT(("Errore. Checksum non corretto per la lunghezza del pacchetto"));
        free(readBuffer);
        return 0;
    }
    ///Controllo che il TFI sia uguale a 25 (frame from PN532 to host)
    if (readBuffer[11]!=0xD5)
    {
        LOG_PRINT(("Errore. TFI diverso da D5"));
        free(readBuffer);
        return 0;
    }
    ///Controllo che il checksum sia corretto
    ///DA FARE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ///Controllo che il postamblo sia zero
    if (readBuffer[12+readBuffer[9]]!=0x00)
    {
        LOG_PRINT(("Errore. Postambolo diverso da 00"));
        free(readBuffer);
        return 0;
    }

    ///Costruisco il buffer che contiene la risposta effettiva del PN532
    if ((readBuffer[9]-1) != r_len)
    {
        LOG_PRINT(("Read length from data read is not as expected, readBuffer[9]=%i, r_len=%i \n",readBuffer[9],r_len));
        free(readBuffer);
        return 0;
    }
    for (i=12;i<12+readBuffer[9]-1;i++)
    {
        readData[i-12] = readBuffer[i];
    }

    free(readBuffer);
    return 1;
}

uint8_t PN532_GetFirmwareVersion(FT_HANDLE ftHandle)
{
    uint8_t WriteData[1];
    uint8_t ReadData[5];

    WriteData[0] = 0x02;
    if(!PN532_WriteCMD(ftHandle,WriteData,1,ReadData,5,0))
        return 0;

    LOG_PRINT(("GetFirmwareVersion\n"));
    LOG_PRINT(("IC      : %02X\n", ReadData[1]));
    LOG_PRINT(("Ver     : %02X\n", ReadData[2]));
    LOG_PRINT(("Rev     : %02X\n", ReadData[3]));
    LOG_PRINT(("Support : %02X\n", ReadData[4]));
    if ((ReadData[4]&0x04)>>2)
        LOG_PRINT(("Standard ISO18092 supportato \n"));
    if ((ReadData[4]&0x02)>>1)
        LOG_PRINT(("Standard ISO/IEC 14443 TypeB supportato \n"));
    if ((ReadData[4]&0x01))
        LOG_PRINT(("Standard ISO/IEC 14443 TypeA supportato \n"));

    return 1;
}

uint8_t PN532_SAMConfiguration(FT_HANDLE ftHandle)
{
    uint8_t WriteData[4];
    uint8_t ReadData[1];

    WriteData[0] = 0x14;
    WriteData[1] = 0x01; ///Normal Mode
    WriteData[2] = 0x00; ///No Timeout
    WriteData[3] = 0x00; ///IRQ at high level
    return PN532_WriteCMD(ftHandle,WriteData,4,ReadData,1,1); ///Immediately wakeup with SAMconfig
}

uint8_t PN532_RFconfiguration(FT_HANDLE ftHandle)
{
    uint8_t WriteData[5];
    uint8_t ReadData[1];

    WriteData[0] = 0x32;
    WriteData[1] = 0x05; ///MaxRetries
    WriteData[2] = 0x00;
    WriteData[3] = 0x01;
    WriteData[4] = 0x02;
    return PN532_WriteCMD(ftHandle,WriteData,5,ReadData,1,0);
}

uint8_t PN532_InlistPassiveTarget(FT_HANDLE ftHandle)
{
    uint8_t WriteData[4];
    uint8_t ReadData[2];

    WriteData[0] = 0x4a;
    WriteData[1] = 0x01;
    WriteData[2] = 0x03;
    WriteData[3] = 0x00;
    return PN532_WriteCMD(ftHandle,WriteData,4,ReadData,2,0);
}

uint8_t PN532_Initiate_Select(FT_HANDLE ftHandle)
{
    uint8_t WriteData[3];
    uint8_t ReadData[3];
    unsigned char chip_id;      ///Chip ID SRIX4K

    ///InCommunicateThru
    WriteData[0] = 0x42;
    WriteData[1] = 0x06;
    WriteData[2] = 0x00;
    if (!PN532_WriteCMD(ftHandle,WriteData,3,ReadData,3,0))
        return 0;

    ///Controllo che lo Status byte sia 0x00
    if (ReadData[1]!=0x00)
    {
        LOG_PRINT(("\nErrore. Comando INITIATE non andato a buon fine \n"));
        return 0;
    }
    chip_id = ReadData[2];

    ///------------------------------------

    ///Invio del comando InCommunicateThru
    ///Scrittura del comando SELECT 0x0e ReadData.packet[2]
    WriteData[0] = 0x42;
    WriteData[1] = 0x0e;
    WriteData[2] = chip_id;
    if (!PN532_WriteCMD(ftHandle,WriteData,3,ReadData,3,0))
        return 0;

    ///Controllo che lo Status byte sia 0x00
    if (ReadData[1]!=0x00)
    {
        LOG_PRINT(("\nErrore. Comando SELECT non andato a buon fine \n"));
        return 0;
    }

    ///Controllo che il Chip ID che ritorna indietro dal comando sia lo stesso di quello ricevuto prima
    if (ReadData[2]!=chip_id)
    {
        LOG_PRINT(("\nErrore. Il Chip ID del comando SELECT non è corretto \n"));
        return 0;
    }

    return 1;
}

uint8_t PN532_ReadBlock(FT_HANDLE ftHandle,uint8_t address, uint32_t *readBlock)
{
    uint8_t WriteData[3];
    uint8_t ReadData[6];

    ///Invio del comando InCommunicateThru
    ///Scrittura del comando READ_BLOCK 0x08 Address
    WriteData[0] = 0x42;
    WriteData[1] = 0x08;
    WriteData[2] = address;
    if (!PN532_WriteCMD(ftHandle,WriteData,3,ReadData,6,0))
        return 0;

    ///Controllo che lo Status byte sia 0x00
    if (ReadData[1]!=0x00)
    {
        LOG_PRINT(("\nErrore. Comando READ_BLOCK non andato a buon fine \n"));
        return 0;
    }

    *readBlock = (ReadData[2] << 24) | (ReadData[3] << 16) | (ReadData[4] << 8) | (ReadData[5]);

    return 1;
}

uint8_t PN532_WriteBlock(FT_HANDLE ftHandle,uint8_t address, uint32_t *writeBlock)
{
    uint8_t WriteData[7];
    uint8_t ReadData[6];

    ///Invio del comando InCommunicateThru
    ///Scrittura del comando WRITE_BLOCK 0x09 Address
    WriteData[0] = 0x42;
    WriteData[1] = 0x09;
    WriteData[2] = address;
    WriteData[3] = (*writeBlock >> 24) & 0xFF;
    WriteData[4] = (*writeBlock >> 16) & 0xFF;
    WriteData[5] = (*writeBlock >>  8) & 0xFF;
    WriteData[6] = (*writeBlock >>  0) & 0xFF;
    if (!PN532_WriteCMD(ftHandle,WriteData,7,ReadData,2,0))
        return 0;

    ///Controllo che lo Status byte sia 0x01, non ho nessuna risposta dallo SRIX4K
    if (ReadData[1]!=0x01)
    {
        LOG_PRINT(("\nErrore. Comando WRITE_BLOCK non andato a buon fine \n"));
        return 0;
    }

    return 1;
}

uint8_t PN532_GetUID(FT_HANDLE ftHandle,uint64_t *UID)
{
    uint8_t WriteData[2];
    uint8_t ReadData[10];

    ///Invio del comando InCommunicateThru
    ///Scrittura del comando GET_UID 0x0b Address
    WriteData[0] = 0x42;
    WriteData[1] = 0x0b;
    if (!PN532_WriteCMD(ftHandle,WriteData,2,ReadData,10,0))
        return 0;

    ///Controllo che lo Status byte sia 0x00
    if (ReadData[1]!=0x00)
    {
        LOG_PRINT(("\nErrore. Comando READ_BLOCK non andato a buon fine \n"));
        return 0;
    }

    *UID = (ReadData[5] << 24) | (ReadData[4] << 16) | (ReadData[3] << 8) | (ReadData[2]);
    *UID = *UID + (ReadData[6] * 4294967296) + (ReadData[7] * 1099511627776) + (ReadData[8] * 281474976710656) + (ReadData[9] * 72057594037927936); ///Lo shift non va...

    return 1;
}

uint8_t PN532_Close(FT_HANDLE ftHandle)
{
    FT_STATUS ftStatus = FT_OK;

    ftStatus = FT_Close(ftHandle);
    if (ftStatus != FT_OK)
    {
        LOG_PRINT(("Errore nella chiusura del dispositivo FTDI"));
        return 0;
    }
    return 1;
}
