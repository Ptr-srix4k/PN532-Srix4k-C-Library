# PN532-Srix4k-C-Library
Libreria in C per leggere le srix4k su Windows usando PN532+FTDI

## Hardware
I collegamenti vanno effettuati in questo modo
| FTDI  | PN532 |
| ----- | ----- |
| VCC   | VCC   |
| GND   | GND   |
| RX    | TXD   |
| TX    | RXD   |

## Istruzioni
- Scaricate i file da questa repository
- Scaricate MSYS2 (https://www.msys2.org/) e seguite tutte le istruzioni nel sito, oppure scaricate Code::Blocks (www.codeblocks.org) nella versione "*mingw-setup.exe"
- Scaricare la libreria FTDI dal sito https://ftdichip.com/drivers/d2xx-drivers/
- Scegliete 32/64 bit in base alla versione di MinGW che usate ed estraete i file
- Copiate il file "ftd2xx.h" nella cartella del progetto
  - Se avete MinGw a 32bit, copiate il file "ftd2xx.lib" dalla cartella "i386" nella cartella del progetto
  - Se avete MinGw a 64bit, copiate il file "ftd2xx.lib" dalla cartella "amd64" nella cartella del progetto
- Usate i seguenti comandi per compilare e linkare il codice:
```
gcc -Wall -c main.c 
gcc -Wall -c srix4k.c 
gcc -o Test.exe main.o srix4k.o -L. -lftd2xx
```

Se usate Code::Blocks vi basta importare i file \*.h e \*.c nel progetto e linkare la libreria FTDI facendo così:  
Project -> Build Options -> Linker settings -> Aggiungete a "Link libraries" la libreria ftd2xx.lib


