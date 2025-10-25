#include <QtWidgets>

#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>

#include <termios.h>
#include <unistd.h>

#include "sio.h"
#include "ptable.h"
#include "signver.h"

void flasher();

// pointer to partition table class
extern ptable_list* ptable;
int32_t signsize;
// pointer to open serial port
extern int siofd; // fd for working with serial port

// data block size transmitted to modem
// #define fblock 4096
#define fblock 2048

//****************************************************
//* Determine flasher version
//*
//*   0 - no response to command
//*   1 - version 2.0
//*  -1 - version not 2.0 
//****************************************************
int dloadversion() {

int res;  
int i;  
QString str;
uint8_t replybuf[1024];

res=atcmd("^DLOADVER?",replybuf);
if (res == 0) return 0; // no response - already HDLC
if (strncmp((char*)replybuf+2,"2.0",3) == 0) return 1;
for (i=2;i<res;i++) {
  if (replybuf[i] == 0x0d) replybuf[i]=0;
}  
str.sprintf("Incorrect flash monitor version - %s",replybuf+2);
QMessageBox::critical(0,"Error",str);
return -1;
}


//***************************************************
// Send partition start command
// 
//  code - 32-bit partition code
//  size - full size of partition to write
// 
//*  result:
//  false - error
//  true - command accepted by modem
//***************************************************
bool dload_start(uint32_t code,uint32_t size) {

uint32_t iolen;  
uint8_t replybuf[4096];
  
static struct __attribute__ ((__packed__)) {
  uint8_t cmd=0x41;
  uint32_t code;
  uint32_t size;
  uint8_t pool[3]={0,0,0};
} cmd_dload_init;

cmd_dload_init.code=htonl(code);
cmd_dload_init.size=htonl(size);
iolen=send_cmd((uint8_t*)&cmd_dload_init,sizeof(cmd_dload_init),replybuf);
if ((iolen == 0) || (replybuf[1] != 2)) return false;
else return true;
}  

//***************************************************
// Send partition block
// 
//  blk - block #
//  pimage - address of partition image start in memory
// 
//*  result:
//  false - error
//  true - command accepted by modem
//***************************************************
bool dload_block(uint32_t part, uint32_t blk, uint8_t* pimage) {

uint32_t res,blksize,iolen;
uint8_t replybuf[4096];

static struct __attribute__ ((__packed__)) {
  uint8_t cmd=0x42;
  uint32_t blk;
  uint16_t bsize;
  uint8_t data[fblock];
} cmd_dload_block;  
  
blksize=fblock; // initial block size value
res=ptable->psize(part)-blk*fblock;  // size of remaining chunk to end of file
if (res<fblock) blksize=res;  // adjust last block size

// block number
cmd_dload_block.blk=htonl(blk+1);
// block size
cmd_dload_block.bsize=htons(blksize);
// data portion from partition image
memcpy(cmd_dload_block.data,pimage+blk*fblock,blksize);
// send block to modem
iolen=send_cmd((uint8_t*)&cmd_dload_block,sizeof(cmd_dload_block)-fblock+blksize,replybuf); // send command

if ((iolen == 0) || (replybuf[1] != 2)) {
  printf("\n sent block:\n");
  dump(&cmd_dload_block,sizeof(cmd_dload_block),0);
  printf("\n\n reply\n");
  dump(replybuf,iolen,0);
  fflush(stdout);
  return false;
}  
else return true;
}

  
//***************************************************
// Complete partition write
// 
//  code - partition code
//  size - partition size
// 
//*  result:
//  false - error
//  true - command accepted by modem
//***************************************************
bool dload_end(uint32_t code, uint32_t size) {

uint32_t iolen;
uint8_t replybuf[4096];

static struct __attribute__ ((__packed__)) {
  uint8_t cmd=0x43;
  uint32_t size;
  uint8_t garbage[3];
  uint32_t code;
  uint8_t garbage1[11];
} cmd_dload_end;

cmd_dload_end.code=htonl(code);
cmd_dload_end.size=htonl(size);
iolen=send_cmd((uint8_t*)&cmd_dload_end,sizeof(cmd_dload_end),replybuf);
if ((iolen == 0) || (replybuf[1] != 2)) {
//   printf("\n sent block:\n");
//   dump(&cmd_dload_end,sizeof(cmd_dload_end),0);
//   printf("\n\n reply\n");
//   dump(replybuf,iolen,0);
//   fflush(stdout);
  return false;
}  
else return true;
}  


//******************************************************************************* 
//* Start flashing process
//******************************************************************************* 
void flasher() {

int32_t res,part;
uint32_t iolen,blk,maxblock;
uint8_t replybuf[4096];
QString txt;
unsigned char cmdver=0x0c;
uint8_t signflag=0,rebootflag=0;


// Modem response options for HDLC commands
unsigned char OKrsp[]={0x0d, 0x0a, 0x4f, 0x4b, 0x0d, 0x0a};
  
QDialog* Flasher=new QDialog;
Flasher->setWindowTitle("Flash modem");
QVBoxLayout* vl=new QVBoxLayout(Flasher);  

QFont font;
font.setPointSize(17);
font.setBold(true);
font.setWeight(75);

QLabel* lbl1=new QLabel("Flash modem");
lbl1->setFont(font);
lbl1->setScaledContents(true);
lbl1->setStyleSheet("QLabel { color : blue; }");


vl->addWidget(lbl1,4,Qt::AlignHCenter);

QCheckBox* dsign = new QCheckBox("Use digital signature",Flasher);
vl->addWidget(dsign);

// check for signature presence and deactivate controls if absent

if (signlen == -1) {
//   printf("\n no sign! \n");
  dsign->setChecked(0);
  dsign->setEnabled(0);
}
else {
  dsign->setChecked(1);
  dsign->setEnabled(1);  
}


QCheckBox* creboot = new QCheckBox("Reboot at the end of flashing",Flasher);
creboot->setChecked(true);
vl->addWidget(creboot);

QDialogButtonBox* buttonBox = new QDialogButtonBox(Flasher);
buttonBox->setOrientation(Qt::Horizontal);
buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
buttonBox->addButton("Start",QDialogButtonBox::AcceptRole);
vl->addWidget(buttonBox,10,Qt::AlignHCenter);

QObject::connect(buttonBox, SIGNAL(accepted()), Flasher, SLOT(accept()));
QObject::connect(buttonBox, SIGNAL(rejected()), Flasher, SLOT(reject()));

// Run dialog
res=Flasher->exec();

// Extract dialog parameters
signflag=dsign->isChecked();
rebootflag=creboot->isChecked();

// Delete current dialog
delete Flasher;
if (res != QDialog::Accepted) return;

Flasher=new QDialog; 

QFormLayout* glm=new QFormLayout(Flasher);

QLabel* pversion = new QLabel(Flasher);
glm->addRow("Protocol version:",pversion);

QLabel* cpart = new QLabel(Flasher);
glm->addRow("Current partition:",cpart);

QProgressBar* partbar = new QProgressBar(Flasher);
partbar->setValue(0);
glm->addRow("Partition:",partbar);

QProgressBar* totalbar = new QProgressBar(Flasher);
totalbar->setValue(0);
glm->addRow("Total:",totalbar);

Flasher->show();
  
// SIO setup
if (!open_port())  {
  QMessageBox::critical(0,"Error","Serial port cannot be opened");
  goto leave;
}  
  
tcflush(siofd,TCIOFLUSH);  // clear output buffer

res=dloadversion();
if (res == -1) {
  QMessageBox::critical(0,"Error","Unsupported flash protocol version");
  goto leave;
}

if (res == 0) {
  QMessageBox::critical(0,"Error","Port is not in flash mode");
  goto leave;
}  

// digital signature
if (signflag) { 
  res=send_signver();
  if (!res) {
    QMessageBox::critical(0,"Error","Digital signature verification error");
    goto leave;
  }  
}  

// Enter HDLC mode
usleep(100000);
res=atcmd("^DATAMODE",replybuf);
if (res != 6) {
  QMessageBox::critical(0,"HDLC entry error","Incorrect response to ^datamode command");
  goto leave;
}  
if (memcmp(replybuf,OKrsp,6) != 0) {
  QMessageBox::critical(0,"HDLC entry error","^datamode command rejected by modem");
  goto leave;
}  

iolen=send_cmd(&cmdver,1,(unsigned char*)replybuf);
if (iolen == 0) {
  QMessageBox::critical(0,"HDLC protocol error","Cannot get flash protocol version");
  goto leave;
}  
// discard initial 7E if present in response
if (replybuf[0] == 0x7e) memcpy(replybuf,replybuf+1,iolen-1);

if (replybuf[0] != 0x0d) {
  QMessageBox::critical(0,"HDLC protocol error","Modem rejected get protocol version command");
  goto leave;
}  

// output protocol version to form
res=replybuf[1];
replybuf[res+2]=0;
txt.sprintf("%s",replybuf+2);
pversion->setText(txt);
QCoreApplication::processEvents();

// Main partition write loop
for(part=0;part<ptable->index();part++) {
 // progress bar by partitions 
 totalbar->setValue(part*100/ptable->index());
 // output partition name to form
 txt.sprintf("%s",ptable->name(part));
 cpart->setText(txt);

 // partition start command
 if (!dload_start(ptable->code(part),ptable->psize(part))) {
  txt.sprintf("Partition %s rejected",ptable->name(part)); 
  QMessageBox::critical(0,"Error",txt);
  goto leave;
}  
    
maxblock=(ptable->psize(part)+(fblock-1))/fblock; // number of blocks in partition
// Block-by-block partition image transmission loop
for(blk=0;blk<maxblock;blk++) {
 // Block progress bar
 partbar->setValue(blk*100/maxblock);
 QCoreApplication::processEvents();

 // Send next block
  if (!dload_block(part,blk,ptable->iptr(part))) {
   txt.sprintf("Block %i of partition %s rejected",blk,ptable->name(part)); 
   QMessageBox::critical(0,"error",txt);
   goto leave;
 }  
}    

// close partition
 if (!dload_end(ptable->code(part),ptable->psize(part))) {
//   txt.sprintf("Error closing partition %s",ptable->name(part)); 
//   QMessageBox::critical(0,"error",txt);
//   leave();
//   return 0;
 }  
} // end of partition loop

// Exit modem from HDLC or reboot if needed
if (rebootflag)   modem_reboot();
else end_hdlc();

totalbar->setValue(100);
partbar->setValue(100);
QCoreApplication::processEvents();
QMessageBox::information(0,"OK","Write completed without errors");

// Complete process
leave:

close_port();

delete Flasher;
 
}
