// kernel partition editor with linux kernel
#include "kerneledit.h"
#include "MainWindow.h"
#include <string.h>
#include "ptable.h"

//********************************************************************
//* Class constructor
//********************************************************************
kerneledit::kerneledit(int xpnum, QWidget* parent) : QWidget(parent) {

QString str;  
char cline[513];
QFont font;
QFont oldfont;
QFont labelfont;
uint8_t* data;
uint32_t len;

pnum=xpnum;

data=ptable->iptr(pnum);
len=ptable->psize(pnum);

// Local copy of partition data
localdata=new uint8_t[len];
memcpy(localdata,data,len);
plen=len-128; // data length minus header

// pointer to the partition header
hdr=(struct boot_img_hdr*)(localdata+128);

// Vertical layout
vlm=new QVBoxLayout(this);

// Get the current font parameters of the labels 
font=QApplication::font("QLabel");
oldfont=font;

// Panel header
font.setPointSize(font.pointSize()+7);
font.setBold(true);
hdrlabel=new QLabel("KERNEL partition editor",this);
hdrlabel->setFont(font);
hdrlabel->setStyleSheet("QLabel { color : green; }");
vlm->addWidget(hdrlabel,0,Qt::AlignHCenter);

// Parameter list header
font=oldfont;
font.setPointSize(font.pointSize()+5);
font.setBold(true);
parmlabel=new QLabel("Kernel parameters",this);
parmlabel->setFont(font);
parmlabel->setStyleSheet("QLabel { color : blue; }");
vlm->addWidget(parmlabel);
vlm->addStretch(1);

// Increase the default font by 2 points
labelfont=oldfont;
labelfont.setPointSize(labelfont.pointSize()+2);

// String layout with parameters
flm=new QFormLayout(0);
vlm->addLayout(flm);

// Page size
str.sprintf("%i",hdr->page_size);
pgslabel=new QLabel(str,this);
pgslabel->setFont(labelfont);
// pgslabel->setFont(labelfont);
flm->addRow("Page size",pgslabel);
flm->labelForField(pgslabel)->setFont(labelfont);

// physical addr for kernel tags
str.sprintf("%08x",hdr->tags_addr);
tagslabel=new QLabel(str,this);
tagslabel->setFont(labelfont);
flm->addRow("physical addr for kernel tags",tagslabel);
flm->labelForField(tagslabel)->setFont(labelfont);

// device tree in bytes
str.sprintf("%08x",hdr->dt_size);
dtlabel=new QLabel(str);
dtlabel->setFont(labelfont);
flm->addRow("physical addr for kernel tags",dtlabel);
flm->labelForField(dtlabel)->setFont(labelfont);

// product name
// strncpy(cline,(char*)(hdr->name),BOOT_NAME_SIZE);
// str=cline;
// pname=new QLabel(str,this);
// flm->addRow("product name",pname);

// boot parameters
strncpy(cline,(char*)hdr->cmdline,BOOT_ARGS_SIZE);
str=cline;
cmdline=new QLineEdit(str,this);
flm->addRow("Boot parameters",cmdline);
flm->labelForField(cmdline)->setFont(labelfont);

vlm->addStretch(1);

// Component list layout
lcomp=new QGridLayout(0);
lcomp->setVerticalSpacing(15);
vlm->addLayout(lcomp);

// table header
font=oldfont;
font.setPointSize(font.pointSize()+3);
font.setBold(true);

comphdr1=new QLabel("Component name",this);
comphdr1->setFont(font);
comphdr1->setStyleSheet("QLabel { color : red; }");
lcomp->addWidget(comphdr1,0,0);

comphdr2=new QLabel("Load address",this);
comphdr2->setFont(font);
comphdr2->setStyleSheet("QLabel { color : orange; }");
lcomp->addWidget(comphdr2,0,1);

comphdr3=new QLabel("Commands",this);
comphdr3->setFont(font);
comphdr3->setStyleSheet("QLabel { color : green; }");
lcomp->addWidget(comphdr3,0,2,1,2,Qt::AlignHCenter);

// component names
kcomp=new QLabel("Kernel image  ",this);
kcomp->setFont(labelfont);
lcomp->addWidget(kcomp,1,0);

r1comp=new QLabel("Ramdisk1",this);
r1comp->setFont(labelfont);
lcomp->addWidget(r1comp,2,0);

r2comp=new QLabel("Ramdisk2",this);
r2comp->setFont(labelfont);
lcomp->addWidget(r2comp,3,0);

// load addresses
str.sprintf("%08x",hdr->kernel_addr);
kaddr=new QLabel(str,this);
kaddr->setFont(labelfont);
lcomp->addWidget(kaddr,1,1,Qt::AlignHCenter);

str.sprintf("%08x",hdr->ramdisk_addr);
r1addr=new QLabel(str,this);
r1addr->setFont(labelfont);
lcomp->addWidget(r1addr,2,1,Qt::AlignHCenter);

str.sprintf("%08x",hdr->second_addr);
r2addr=new QLabel(str,this);
r2addr->setFont(labelfont);
lcomp->addWidget(r2addr,3,1,Qt::AlignHCenter);

// extract buttons
kext=new QPushButton("Extract",this);
connect(kext,SIGNAL(clicked()),this,SLOT(kextract()));
lcomp->addWidget(kext,1,2);

r1ext=new QPushButton("Extract",this);
connect(r1ext,SIGNAL(clicked()),this,SLOT(r1extract()));
lcomp->addWidget(r1ext,2,2);

r2ext=new QPushButton("Extract",this);
connect(r2ext,SIGNAL(clicked()),this,SLOT(r2extract()));
lcomp->addWidget(r2ext,3,2);

// replace buttons
krepl=new QPushButton("Replace",this);
connect(krepl,SIGNAL(clicked()),this,SLOT(kreplace()));
lcomp->addWidget(krepl,1,3);

r1repl=new QPushButton("Replace",this);
connect(r1repl,SIGNAL(clicked()),this,SLOT(r1replace()));
lcomp->addWidget(r1repl,2,3);

r2repl=new QPushButton("Replace",this);
connect(r2repl,SIGNAL(clicked()),this,SLOT(r2replace()));
lcomp->addWidget(r2repl,3,3);

// right spacer
rspacer=new QSpacerItem(100,10,QSizePolicy::Expanding);
lcomp->addItem(rspacer,1,4);

vlm->addStretch(7);

}

//********************************************************************
//* Class destructor
//********************************************************************
kerneledit::~kerneledit() {

QMessageBox::StandardButton reply;
QString cmd;

// check the command line editor 
if (cmdline->isModified()) {
  cmd=cmdline->text();
  strncpy((char*)hdr->cmdline,cmd.toLocal8Bit().data(),BOOT_ARGS_SIZE);
}  

// check if the data has changed
if ((ptable->psize(pnum) != (plen+128)) || (memcmp(localdata,ptable->iptr(pnum),plen+128) != 0)) {
  reply=QMessageBox::warning(this,"Write partition","The content of the partition has been changed, save?",QMessageBox::Ok | QMessageBox::Cancel);
  if (reply == QMessageBox::Ok) {
    ptable->replace(pnum,localdata,plen+128);
  }
}  
delete localdata;  
}

//********************************************************************
//* Calculating the component address
//*   0 - kernel
//*   1 - ramdisk 1
//*   2 - ramdisk 2
//********************************************************************
void kerneledit::setup_adr(int type, uint32_t* adr, uint32_t* rlen, QString* filename=0) {
uint32_t start=0;
uint32_t size=0;

uint32_t kernelsize=(hdr->kernel_size+hdr->page_size-1)/hdr->page_size;
uint32_t ram1size=(hdr->ramdisk_size+hdr->page_size-1)/hdr->page_size;

switch(type) {
  case 0:
    start=1;
    size=hdr->kernel_size;
    if (filename != 0) *filename="vmlinuz.bin";
    break;
    
  case 1:
    start=kernelsize+1;
    size=hdr->ramdisk_size;
    if (filename != 0) *filename="ramdisk1.cpio.gz";
    break;
    
  case 2:
    start=kernelsize+ram1size+1;
    size=hdr->second_size;
    if (filename != 0) *filename="ramdisk2.cpio.gz";
    break;
}
// convert from pages to bytes
start*=hdr->page_size;
*adr=start;
*rlen=size;
}


//********************************************************************
//* Extracting components
//*   0 - kernel
//*   1 - ramdisk 1
//*   2 - ramdisk 2
//********************************************************************
void kerneledit::extractor(int type) {

uint32_t start;
uint32_t size;
QString filename;

setup_adr(type,&start,&size,&filename);
filename=QFileDialog::getSaveFileName(this,"Saved file name",filename,"All files (*.*)");
if (filename.isEmpty()) return;

QFile out(filename,this);
if (!out.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(0,"Error","File creation error");
    return;
}
out.write((char*)(localdata+128+start),size);
out.close();
}


//********************************************************************
//* Slots for extracting component images
//********************************************************************
void kerneledit::kextract() { extractor(0); }
void kerneledit::r1extract() { extractor(1); }
void kerneledit::r2extract() { extractor(2); }

//********************************************************************
//* Replacing components
//*   0 - kernel
//*   1 - ramdisk 1
//*   2 - ramdisk 2
//********************************************************************
void kerneledit::replacer(int type) {

QString filename="";
uint32_t kernelsize,r1size,r2size;
uint32_t bound_filesize, fsize;
uint32_t totalsize;
uint32_t pagesize=hdr->page_size;
uint8_t* newlocaldata;
uint8_t* srcptr;
uint8_t* dstptr;

// component sizes aligned to the flash page boundary
kernelsize=(hdr->kernel_size+pagesize-1)/pagesize*pagesize;
r1size=(hdr->ramdisk_size+pagesize-1)/pagesize*pagesize;
r2size=(hdr->second_size+pagesize-1)/pagesize*pagesize;

// file selection
filename=QFileDialog::getOpenFileName(this,"File name",filename,"All files (*.*)");
if (filename.isEmpty()) return;

QFile out(filename,this);
if (!out.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(0,"Error","File read error");
    return;
}

// Read the component image from the file
fsize=out.size();
bound_filesize=(fsize+pagesize-1)/pagesize*pagesize; // rounded up to the page
char* fbuf=new char[bound_filesize]; // file buffer
bzero(fbuf,bound_filesize);
out.read(fbuf,fsize);
out.close();

// Calculate the new partition size
totalsize=pagesize;
if (type == 0) totalsize+=bound_filesize;  else totalsize+=kernelsize;
if (type == 1) totalsize+=bound_filesize;  else totalsize+=r1size;
if (type == 2) totalsize+=bound_filesize;  else totalsize+=r2size;

// Allocate memory for the new partition image
newlocaldata=new uint8_t[totalsize+128];

// copy the huawei and android header
memcpy(newlocaldata,localdata,128+pagesize);
// reconfigure the pointer to the android header
hdr=(struct boot_img_hdr*)(newlocaldata+128);

// set up the source-receiver pointers
srcptr=localdata+pagesize+128;
dstptr=newlocaldata+pagesize+128;

// copy partitions
//------------------------
// kernel
if (type == 0) {
  memcpy(dstptr,fbuf,bound_filesize);
  srcptr+=kernelsize;
  dstptr+=bound_filesize;
  hdr->kernel_size=fsize;
}  
else {
  memcpy(dstptr,srcptr,kernelsize);
  srcptr+=kernelsize;
  dstptr+=kernelsize;
}

// ramdisk 1
if (type == 1) {
  memcpy(dstptr,fbuf,bound_filesize);
  srcptr+=r1size;
  dstptr+=bound_filesize;
  hdr->ramdisk_size=fsize;
  
}  
else {
  memcpy(dstptr,srcptr,r1size);
  srcptr+=r1size;
  dstptr+=r1size;
}

// ramdisk 2
if (type == 2) {
  memcpy(dstptr,fbuf,bound_filesize);
  srcptr+=r2size;
  dstptr+=bound_filesize;
  hdr->second_size=fsize;
}  
else {
  memcpy(dstptr,srcptr,r2size);
  srcptr+=r2size;
  dstptr+=r2size;
}

// delete the file buffer
delete fbuf;
// Delete the old buffer and put a new one in its place

delete localdata;
localdata=newlocaldata;
plen=totalsize;

}

//********************************************************************
//* Slots for replacing component images
//********************************************************************
void kerneledit::kreplace() { replacer(0); }
void kerneledit::r1replace() { replacer(1); }
void kerneledit::r2replace() { replacer(2); }
