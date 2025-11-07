#include <QtWidgets>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <termios.h>
#include <unistd.h>

#include "sio.h"
#include "usbloader.h"
#include "ulpatcher.h"

// pointer to the open serial port
extern int siofd; // fd for working with the Serial port


//*************************************************
//* Calculation of the checksum of the command packet
//*************************************************
void csum(unsigned char* buf, uint32_t len) {

unsigned  int i,c,csum=0;

unsigned int cconst[]={0,0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF};

for (i=0;i<len;i++) {
  c=(buf[i]&0xff);
  csum=((csum<<4)&0xffff)^cconst[(c>>4)^(csum>>12)];
  csum=((csum<<4)&0xffff)^cconst[(c&0xf)^(csum>>12)];
}  
buf[len]=(csum>>8)&0xff;
buf[len+1]=csum&0xff;
  
}

//*************************************************
//*   Sending a command packet to the modem
//*************************************************
int sendcmd(void* srcbuf, int len) {

unsigned char replybuf[1024];
unsigned char cmdbuf[2048];
unsigned int replylen;

// local copy of the command buffer
memcpy(cmdbuf,srcbuf,len);

// add a checksum to it
csum(cmdbuf,len);

// send command
write(siofd,cmdbuf,len+2);  
tcdrain(siofd);

// read the answer
replylen=read(siofd,replybuf,1024);

if (replylen == 0) return 0;     // empty answer
if (replybuf[0] == 0xaa) return 1; // correct answer
return 0;
}

//*************************************
//* Search for the linux kernel in the partition image
//*************************************
int locate_kernel(uint8_t* pbuf, uint32_t size) {
  
int off;

for(off=(size-8);off>0;off--) {
  if (strncmp((char*)(pbuf+off),"ANDROID!",8) == 0) return off;
}
return 0;
}

//*********************************************
//* Search for the partition table in the bootloader 
//*********************************************
uint32_t find_ptable(uint8_t* buf, uint32_t size) {

// table header signature  
const uint8_t headmagic[16]={0x70, 0x54, 0x61, 0x62, 0x6c, 0x65, 0x48, 0x65, 0x61, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};  
uint32_t off;

for(off=0;off<(size-16);off+=4) {
  if (memcmp(buf+off,headmagic,16) == 0)   return off;
}
return 0;
}


//***************************************
//* Partition table patch
//***************************************
int ptable_patch(char* filename, uint8_t* pbuf[], struct lhead* part) {

FILE* in;
uint32_t fsize,ptoff;
char ptbuf[0x800];

in=fopen(filename,"r");
if (in == 0) {
  QMessageBox::critical(0,"Error","Error opening file");
  return 0;
}

// load the file into the buffer
fsize=fread(ptbuf,1,0x800,in);
fclose(in);
if (fsize != 0x800) {
  QMessageBox::critical(0,"Error","File is too short");
  return 0;
}  
if (strncmp((char*)ptbuf,"pTableHead",10) != 0) {
  QMessageBox::critical(0,"Error","The file is not a partition table");
  return 0;
}  

// look for the partition table inside the bootloader
ptoff=find_ptable(pbuf[1], part[1].size);
if (ptoff == 0) {
  QMessageBox::critical(0,"Error","The built-in partition table was not found in the bootloader");
  return 0;
}  
// replace the partition table
memcpy(pbuf[1]+ptoff,ptbuf,0x800);
return 1;
}



//***************************************
//* Select bootloader file
//***************************************
void usbldialog::browse() {

QString name;  
name=QFileDialog::getOpenFileName(this,"Select bootloader file",".","usbloader (*.bin);;All files (*.*)");
fname->setText(name);
}

//***************************************
//* Select partition table file
//***************************************
void usbldialog::ptbrowse() {

QString name;  
name=QFileDialog::getOpenFileName(this,"Select partition table file",".","usbloader (*.bin);;All files (*.*)");
ptfname->setText(name);
}

//***************************************
//* Clearing the partition table file name
//***************************************
void usbldialog::ptclear() {

ptfname->setText("");
}

//***************************************
// fastboot-patch
//***************************************
int fastboot_only(uint8_t* pbuf[], struct lhead* part) {

int koff;  // offset to the ANDROID-header

koff=locate_kernel(pbuf[1],part[1].size);
if (koff != 0) {
      *(pbuf[1]+koff)=0x55; // signature patch
      part[1].size=koff+8; // cut the partition to the beginning of the kernel
      return 1;
}

QMessageBox::critical(0,"Error"," There is no ANDROID component in the bootloader - fastboot loading is not possible");
return 0;
  
}


//***************************************
//* Sending the component header
//***************************************
int start_part(uint32_t size,uint32_t adr,uint8_t lmode) {
  
struct __attribute__ ((__packed__)) {
  uint8_t cmd[3]={0xfe,0, 0xff};
  uint8_t lmode;
  uint32_t size;
  uint32_t adr;
}  cmdhead;

cmdhead.size=htonl(size);
cmdhead.adr=htonl(adr);
cmdhead.lmode=lmode;
  
return sendcmd(&cmdhead,sizeof(cmdhead));
}  
  
    
//***************************************
//* Sending a data packet
//***************************************
int send_data_packet(uint32_t pktcount, uint8_t* databuf, uint32_t datasize) {

// packet image
struct __attribute__ ((__packed__)) {
 uint8_t cmd=0xda; 
 uint8_t count;
 uint8_t rcount;
 uint8_t data[2048];
} cmddata;
  
cmddata.count=pktcount&0xff;
cmddata.rcount=(~pktcount)&0xff;
memcpy(cmddata.data,databuf,datasize);

return sendcmd(&cmddata,datasize+3);
}  


//***************************************
//* Closing the component data stream
//***************************************
int close_part(uint32_t pktcount) {

struct __attribute__ ((__packed__)) {
 uint8_t cmd=0xed;
 uint8_t count;
 uint8_t rcount;
} cmdeod; 
  
// Form a data end packet
cmdeod.count=pktcount&0xff;;
cmdeod.rcount=(~pktcount)&0xff;

return sendcmd(&cmdeod,sizeof(cmdeod));
}

//***************************************
//* Start loading
//***************************************
void usbload() {

// bootloader component directory storage
struct lhead part[5];

// array of buffers for loading components
uint8_t* pbuf[5]={0,0,0,0,0};

uint16_t numparts; // number of components to load
  
uint32_t bl,datasize,pktcount;
uint32_t adr,i,fsize,totalsize=0,loadedsize=0;
uint8_t c;
int32_t res;
int32_t pflag,fflag,bflag;
// file names - declared static and saved when the dialog is reloaded
static char filename[200]={0};
static char ptfilename[200]={0};

FILE* in;

usbldialog* qd=new usbldialog;
qd->setWindowTitle("Loading usbloader");
QVBoxLayout* vl=new QVBoxLayout(qd);

QFont font;
font.setPointSize(17);
font.setBold(true);
font.setWeight(75);

QLabel* lbl1=new QLabel("USB BOOT");
lbl1->setFont(font);
lbl1->setScaledContents(true);
lbl1->setStyleSheet("QLabel { color : blue; }");

vl->addWidget(lbl1,4,Qt::AlignHCenter);

// nested lm for file selectors
QGridLayout* gvl=new QGridLayout(0);
vl->addLayout(gvl);

QLabel* lbl2=new QLabel("usbloader:");
gvl->addWidget(lbl2,0,0);

qd->fname=new QLineEdit(qd);
if (strlen(filename) != 0) qd->fname->setText(filename);
gvl->addWidget(qd->fname,0,1);

QToolButton* fselector = new QToolButton(qd);
// fselector->setText("...");
fselector->setIcon(QIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon))); 
gvl->addWidget(fselector,0,2);

QLabel* lbl3=new QLabel("Partition table:");
gvl->addWidget(lbl3,1,0);

qd->ptfname=new QLineEdit(qd);
if (strlen(ptfilename) != 0) qd->ptfname->setText(ptfilename);
gvl->addWidget(qd->ptfname,1,1);

QToolButton* ptselector = new QToolButton(qd);
// ptselector->setText("...");
ptselector->setIcon(QIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon))); 
gvl->addWidget(ptselector,1,2);

QToolButton* ptclear = new QToolButton(qd);
// ptclear->setText("X");
ptclear->setIcon(QIcon(QApplication::style()->standardIcon(QStyle::SP_TrashIcon))); 
gvl->addWidget(ptclear,1,3);

// boot mode selection buttons
QCheckBox* fbflag = new QCheckBox("Loading in FASTBOOT mode",qd);
vl->addWidget(fbflag);

QCheckBox* isbadflag= new QCheckBox("Disable bad block control",qd);
vl->addWidget(isbadflag);

QCheckBox* patchflag= new QCheckBox("Disable eraseall patch (DANGEROUS!!!)",qd);
vl->addWidget(patchflag);

QDialogButtonBox* buttonBox = new QDialogButtonBox(qd);
buttonBox->setOrientation(Qt::Horizontal);
buttonBox->addButton("Cancel",QDialogButtonBox::RejectRole);
buttonBox->addButton("Loading",QDialogButtonBox::AcceptRole);
vl->addWidget(buttonBox,10,Qt::AlignHCenter);

QObject::connect(buttonBox, SIGNAL(accepted()), qd, SLOT(accept()));
QObject::connect(buttonBox, SIGNAL(rejected()), qd, SLOT(reject()));
QObject::connect(fselector, SIGNAL(clicked()), qd, SLOT(browse()));
QObject::connect(ptselector, SIGNAL(clicked()), qd, SLOT(ptbrowse()));
QObject::connect(ptclear, SIGNAL(clicked()), qd, SLOT(ptclear()));

// Start the dialog
res=qd->exec();

// extract data from the dialog
fflag=fbflag->isChecked();
pflag=patchflag->isChecked();
bflag=isbadflag->isChecked();
strcpy(filename,qd->fname->displayText().toLocal8Bit());
strcpy(ptfilename,qd->ptfname->displayText().toLocal8Bit());

// delete the dialog panel
delete qd;

if (res != QDialog::Accepted) return;

//--------- Reading the bootloader into memory ---------------

// open the bootloader file
in=fopen(filename,"r");
if (in == 0) {
  QMessageBox::critical(0,"Error","Error opening file");
  return;
}  
  

// Check the usloader signature
fread(&i,1,4,in);
if (i != 0x20000) {
  QMessageBox::critical(0,"Error","The file is not a usbloader bootloader");
  fclose(in);
  return;
}  

// read the bootloader header
fseek(in,36,SEEK_SET); // beginning of the component catalog in the file

fread(&part,sizeof(part),1,in);

// Search for the end of the component catalog
for (i=0;i<5;i++) {
  if (part[i].lmode == 0) break;
}
numparts=i;

// Load components into memory
for(i=0;i<numparts;i++) {
 // go to the beginning of the component image
 fseek(in,part[i].offset,SEEK_SET);
 // free the previous distributed buffer
 if (pbuf[i] != 0) {
   free(pbuf[i]);
   pbuf[i]=0;
 }  
 // read the entire component into the buffer
 pbuf[i]=(uint8_t*)malloc(part[i].size);
 fsize=fread(pbuf[i],1,part[i].size,in);
 if (part[i].size != fsize) {
      QMessageBox::critical(0,"Error","Unexpected end of file");
      fclose(in);
      return;
 }
 // total bootloader size
 totalsize+=part[i].size;
}

fclose(in);

// do fastboot-patch
if (fflag) {
  if (!fastboot_only(pbuf,part)) return;
}  

// ERASE-patch
if (!pflag) {
  res=pv7r2(pbuf[1], part[1].size)+ pv7r11(pbuf[1], part[1].size) + pv7r1(pbuf[1], part[1].size) + pv7r22(pbuf[1], part[1].size) + pv7r22_2(pbuf[1], part[1].size);
  if (res == 0)  {
   QMessageBox::critical(0,"Error","Patch signature not found, loading not performed");
   return;
  }  
}  

// isbad-patch
if (bflag) {
  res=perasebad(pbuf[1], part[1].size);
  if (res == 0)  {
   QMessageBox::critical(0,"Error","BAD ERASE signature not found, loading not performed");
   return;
  }  
}  


// Replace the partition table
if (strlen(ptfilename) != 0) {
 if (!ptable_patch(ptfilename, pbuf, part)) return;
} 

//-------------------------------------------------------------------  
// SIO setup
if (!open_port())  {
  QMessageBox::critical(0,"Error","Serial port does not open");
  return;
}  


// Check the boot port
c=0;
write(siofd,"A",1);   // send an arbitrary byte to the port
bl=read(siofd,&c,1);
// the answer should be U (0x55)
if (c != 0x55) {
  QMessageBox::critical(0,"Error","The serial port is not in USB Boot mode");
  close_port();
  return;
}  


// Form the indicator panel
QDialog* ind=new QDialog;
QFormLayout* lmf=new QFormLayout(ind);

QProgressBar* partbar = new QProgressBar(ind);
partbar->setValue(0);
lmf->addRow("Partition:",partbar);

QProgressBar* totalbar = new QProgressBar(ind);
totalbar->setValue(0);
lmf->addRow("Total:",totalbar);

ind->show();

// main loading cycle - load all blocks found in the header

for(bl=0;bl<numparts;bl++) {

  
 // starting package  
 if (!start_part(part[bl].size,part[bl].adr,part[bl].lmode)) {
   QMessageBox::critical(0,"Error","The modem rejected the component header");
   goto leave;
 }  

  // Data block loading cycle
  datasize=1024;
  pktcount=1;
  for(adr=0;adr<part[bl].size;adr+=1024) {
    // check for the last block of the component
    if ((adr+1024)>=part[bl].size) datasize=part[bl].size-adr; 
     
    // update the block progress bar 
    partbar->setValue(adr*100/part[bl].size);            // for the partition
    totalbar->setValue((loadedsize+adr)*100/totalsize);  // total
    QCoreApplication::processEvents();
    
    if (!send_data_packet(pktcount++,(uint8_t*)(pbuf[bl]+adr),datasize)) {
      QMessageBox::critical(0,"Error","The modem rejected the data packet");
      goto leave;
    }  
  }
  // update the size of the already loaded data
  loadedsize+=part[bl].size;


  if (!close_part(pktcount)) {
      QMessageBox::critical(0,"Error","The modem rejected the component end command");
      goto leave;
    }  
} 

totalbar->setValue(100);
partbar->setValue(100);
QCoreApplication::processEvents();
      
QMessageBox::information(0,"OK","Loading finished");

leave:
close_port();
delete ind;

}

  