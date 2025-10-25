#include <QtWidgets>

// Digital signature processing procedures
// 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "sio.h"
#include "ptable.h"
#include "MainWindow.h"
// #include "flasher.h"
// #include "util.h"
// #include "zlib.h"


#define signbaselen 6


// resulting ^signver command string
uint8_t signver[200];

// Firmware type flag
extern int dflag;

// Current digital signature parameters
uint32_t signtype; // firmware type
int32_t signlen=-1;  // signature length

int32_t serach_sign();

// Public key hash for ^signver
char signver_hash[100]="778A8D175E602B7B779D9E05C330B5279B0661BF2EED99A20445B366D63DD697";



  

//***************************************************
//* Send digital signature
//***************************************************
int32_t send_signver() {
  
uint32_t res;
// ^signver response
unsigned char SVrsp[]={0x0d, 0x0a, 0x30, 0x0d, 0x0a, 0x0d, 0x0a, 0x4f, 0x4b, 0x0d, 0x0a};
uint8_t replybuf[200];
char message[100];  

signtype=dload_id&0x7;

sprintf((char*)signver,"^SIGNVER=%i,0,%s,%i",signtype,signver_hash,signlen);
res=atcmd((char*)signver,replybuf);
if ( (res<sizeof(SVrsp)) || (memcmp(replybuf,SVrsp,sizeof(SVrsp)) != 0) ) {
   sprintf(message,"Digital signature verification error - %02x",replybuf[2]);
   QMessageBox::critical(0,"Error",message);
   return -2;
}
return 1;
}

//***************************************************
//* Search for digital signature in firmware
//***************************************************
int32_t search_sign() {

int i,j;
uint32_t pt;
uint8_t* imageptr;

// search in partitions 0 and 1
for (i=0;i<2;i++) {
  if (ptable->index() == i) break;
  imageptr=ptable->iptr(i)+ptable->psize(i);
  pt=*(uint32_t*)(imageptr-4);
  if (pt == 0xffaaaffa) { 
    // signature found
    signlen=*(uint32_t*)(imageptr-12);
    bzero(signver_hash,100);
    // extract public key hash
//     printf("\n psize = %08x",
    for(j=0;j<32;j++) {
     sprintf(signver_hash+2*j,"%02X",*(imageptr-signlen+6+j));
    }
    printf("\n hash = %s",signver_hash);
    return signlen;
  }
}
// not found
return -1;
}
 
//********************************************
//* Display digital signature information
//********************************************
void MainWindow::ShowSignInfo() {

QDialog* sd=new QDialog;

QDialogButtonBox* btn=new QDialogButtonBox(QDialogButtonBox::Ok,sd);

QFormLayout* lm=new QFormLayout(sd);

char str[200];

QLabel* dlid=new QLabel(fw_description(dload_id));
lm->addRow("Firmware type",dlid);

sprintf(str,"%i",signlen);
QLabel* signln=new QLabel(str);
lm->addRow("Signature size",signln);

QLabel* hash=new QLabel(signver_hash);
lm->addRow("Key hash",hash);

lm->addRow(0,btn);

connect(btn,SIGNAL(accepted()),sd,SLOT(accept()));
sd->exec();

delete btn;
delete dlid;
delete hash;
delete signln; 
delete lm;
delete sd;
}
