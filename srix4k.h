//#define DEBUG
//#define LOG

#ifdef LOG
# define LOG_PRINT(x) printf x
#else
# define LOG_PRINT(x) do {} while (0)
#endif

struct DataPack
{
  unsigned char *packet;
  unsigned char length;
};

extern uint8_t PN532_Init(FT_HANDLE *ftHandle);
extern uint8_t PN532_WriteCMD(FT_HANDLE ftHandle,uint8_t *writeData, uint8_t w_len, uint8_t *readData, uint8_t r_len, uint8_t wakeup);
extern uint8_t PN532_GetFirmwareVersion(FT_HANDLE ftHandle);
extern uint8_t PN532_SAMConfiguration(FT_HANDLE ftHandle);
extern uint8_t PN532_RFconfiguration(FT_HANDLE ftHandle);
extern uint8_t PN532_InlistPassiveTarget(FT_HANDLE ftHandle);
extern uint8_t PN532_Initiate_Select(FT_HANDLE ftHandle);
extern uint8_t PN532_ReadBlock(FT_HANDLE ftHandle,uint8_t address, uint32_t *readBlock);
extern uint8_t PN532_WriteBlock(FT_HANDLE ftHandle,uint8_t address, uint32_t *writeBlock);
extern uint8_t PN532_GetUID(FT_HANDLE ftHandle,uint64_t *UID);
extern uint8_t PN532_Close(FT_HANDLE ftHandle);
