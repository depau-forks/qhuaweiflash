// nvdload partition editor
#include "nvdedit.h"
#include "MainWindow.h"
#include <string.h>
#include "ptable.h"
#include "viewer.h"
#include "nvexplorer.h"

//********************************************************************
//* Class constructor
//********************************************************************
nvdedit::nvdedit(int xpnum, QWidget* parent) : QWidget(parent) {

QString str;  
QFont font;
QFont oldfont;
QFont labelfont;

pnum=xpnum;

// Local copy of partition data
data=new uint8_t[ptable->psize(pnum)];
plen=ptable->psize(pnum);
memcpy(data,ptable->iptr(pnum),plen);

// partition header
memcpy(&hdr,data,sizeof(hdr));

// file type
if (hdr.nv_bin.off == sizeof(hdr)) filetype=2; // full header - new chipsets
   else filetype=1;   // partial header - old chipsets
    
// partition components
//  nv.bin
nvpart=new uint8_t[hdr.nv_bin.len];
memcpy(nvpart,data+hdr.nv_bin.off,hdr.nv_bin.len);
// main xml
if ((hdr.xnv_xml).magic == NV_FILE_MAGIC) {
 xmlpart=new uint8_t[hdr.xnv_xml.len];
 memcpy(xmlpart,data+hdr.xnv_xml.off,hdr.xnv_xml.len);
}
// additional xml
if (hdr.cust_xml.magic == NV_FILE_MAGIC) {
 custxmlpart=new uint8_t[hdr.cust_xml.len];
 memcpy(custxmlpart,data+hdr.cust_xml.off,hdr.cust_xml.len);
}
// xml map
if (hdr.xnv_map.magic == NV_FILE_MAGIC) {
 xmlmap=new uint8_t[hdr.xnv_map.len];
 memcpy(xmlmap,data+hdr.xnv_map.off,hdr.xnv_map.len);
}


// Vertical layout
vlm=new QVBoxLayout(this);

// Get the current font parameters of the labels 
font=QApplication::font("QLabel");
oldfont=font;

// Panel header
font.setPointSize(font.pointSize()+7);
font.setBold(true);
hdrlabel=new QLabel("NVDLOAD partition editor",this);
hdrlabel->setFont(font);
hdrlabel->setStyleSheet("QLabel { color : green; }");
vlm->addWidget(hdrlabel,0,Qt::AlignHCenter);

// Increase the default font by 2 points
labelfont=oldfont;
labelfont.setPointSize(labelfont.pointSize()+2);

// File type
if (filetype == 1) str = "NVDLOAD structure type: 1 (V7R11 chipset and older)";
else str = "NVDLOAD structure type: 2 (V7R22 chipset and newer)";
hdrlabel=new QLabel(str,this);
hdrlabel->setFont(labelfont);
hdrlabel->setStyleSheet("QLabel { color : blue; }");
vlm->addWidget(hdrlabel);
vlm->addStretch(1);

// Component list layout
lcomp=new QGridLayout(0);
lcomp->setVerticalSpacing(15);
vlm->addLayout(lcomp);

// table header
font=oldfont;
font.setPointSize(font.pointSize()+3);
font.setBold(true);

comphdr1=new QLabel("Component  ",this);
comphdr1->setFont(font);
comphdr1->setStyleSheet("QLabel { color : red; }");
lcomp->addWidget(comphdr1,0,0);

comphdr2=new QLabel("Size",this);
comphdr2->setFont(font);
comphdr2->setStyleSheet("QLabel { color : orange; }");
lcomp->addWidget(comphdr2,0,1);

comphdr3=new QLabel("Commands",this);
comphdr3->setFont(font);
comphdr3->setStyleSheet("QLabel { color : green; }");
lcomp->addWidget(comphdr3,0,2,1,2,Qt::AlignHCenter);

// component names
name1=new QLabel("NVIMG",this);
name1->setFont(labelfont);
lcomp->addWidget(name1,1,0);

name2=new QLabel("Base XML",this);
name2->setFont(labelfont);
lcomp->addWidget(name2,2,0);

name3=new QLabel("Custom XML",this);
name3->setFont(labelfont);
lcomp->addWidget(name3,3,0);

name4=new QLabel("XML MAP",this);
name4->setFont(labelfont);
lcomp->addWidget(name4,4,0);

// component sizes
str.sprintf("%i",hdr.nv_bin.len);
size1=new QLabel(str,this);
size1->setFont(labelfont);
lcomp->addWidget(size1,1,1,Qt::AlignHCenter);

str.sprintf("%i",hdr.xnv_xml.len);
size2=new QLabel(str,this);
size2->setFont(labelfont);
lcomp->addWidget(size2,2,1,Qt::AlignHCenter);

str.sprintf("%i",hdr.cust_xml.len);
size3=new QLabel(str,this);
size3->setFont(labelfont);
lcomp->addWidget(size3,3,1,Qt::AlignHCenter);

str.sprintf("%i",hdr.xnv_map.len);
size3=new QLabel(str,this);
size3->setFont(labelfont);
lcomp->addWidget(size3,4,1,Qt::AlignHCenter);


// extract buttons 
extr1=new QPushButton("Extract",this);
connect(extr1,SIGNAL(clicked()),this,SLOT(extract1()));
lcomp->addWidget(extr1,1,2);

if (hdr.xnv_xml.len != 0) {
 extr2=new QPushButton("Extract",this);
 connect(extr2,SIGNAL(clicked()),this,SLOT(extract2()));
 lcomp->addWidget(extr2,2,2);
}

if (hdr.cust_xml.len != 0) {
 extr3=new QPushButton("Extract",this);
 connect(extr3,SIGNAL(clicked()),this,SLOT(extract3()));
 lcomp->addWidget(extr3,3,2);
}

if (hdr.xnv_map.len != 0) {
 extr4=new QPushButton("Extract",this);
 connect(extr4,SIGNAL(clicked()),this,SLOT(extract4()));
 lcomp->addWidget(extr4,4,2);
}

// replace buttons
repl1=new QPushButton("Replace",this);
connect(repl1,SIGNAL(clicked()),this,SLOT(replace1()));
lcomp->addWidget(repl1,1,3);

if (hdr.xnv_xml.len != 0) {
 repl2=new QPushButton("Replace",this);
 connect(repl2,SIGNAL(clicked()),this,SLOT(replace2()));
 lcomp->addWidget(repl2,2,3);
}

if (hdr.cust_xml.len != 0) {
 repl3=new QPushButton("Replace",this);
 connect(repl3,SIGNAL(clicked()),this,SLOT(replace3()));
 lcomp->addWidget(repl3,3,3);
}

if (hdr.xnv_map.len != 0) {
 repl4=new QPushButton("Replace",this);
 connect(repl4,SIGNAL(clicked()),this,SLOT(replace4()));
 lcomp->addWidget(repl4,4,3);
}

// edit buttons

edit1=new QPushButton("Edit",this);
connect(edit1,SIGNAL(clicked()),this,SLOT(nvexpl()));
lcomp->addWidget(edit1,1,4);


if (hdr.xnv_xml.len != 0) {
 edit2=new QPushButton("Edit",this);
 connect(edit2,SIGNAL(clicked()),this,SLOT(xedit2()));
 lcomp->addWidget(edit2,2,4);
}

if (hdr.cust_xml.len != 0) {
 edit3=new QPushButton("Edit",this);
 connect(edit3,SIGNAL(clicked()),this,SLOT(xedit3()));
 lcomp->addWidget(edit3,3,4);
}

// right spacer
rspacer=new QSpacerItem(100,10,QSizePolicy::Expanding);
lcomp->addItem(rspacer,1,5);

vlm->addStretch(7);
}

//********************************************************************
//* Class destructor
//********************************************************************
nvdedit::~nvdedit() {

QMessageBox::StandardButton reply;
QString cmd;
 
// reassemble the data
if (changed) rebuild_data();

// check if the data has changed
if ((ptable->psize(pnum) != plen) || (memcmp(data,ptable->iptr(pnum),plen) != 0)) {
  reply=QMessageBox::warning(this,"Write partition","The content of the partition has been changed, save?",QMessageBox::Ok | QMessageBox::Cancel);
  if (reply == QMessageBox::Ok) {
    ptable->replace(pnum,data,plen);
  }
}  
delete [] data;
delete nvpart;
if (xmlpart != 0) delete [] xmlpart;
if (custxmlpart != 0) delete [] custxmlpart;
if (xmlmap != 0) delete [] xmlmap;

}

//********************************************************************
//* Extracting components
//*   0 - NVIMG
//*   1 - Base XML
//*   2 - Custom XML
//*   3 - XNV MAP
//********************************************************************
void nvdedit::extractor(int type) {

// default file names
char* compnames[4]= {
  "nvimg.nvm",
  "base.xml",
  "custom.xml",
  "xnvmap.bin"
};  
uint32_t start=0,len=0;

QString filename=compnames[type];

switch(type) {
  case 0:
    start=hdr.nv_bin.off;
    len=hdr.nv_bin.len;
    break;
    
  case 1:
    start=hdr.xnv_xml.off;
    len=hdr.xnv_xml.len;
    break;
    
  case 2:
    start=hdr.cust_xml.off;
    len=hdr.cust_xml.len;
    break;
    
  case 3:
    start=hdr.xnv_map.off;
    len=hdr.xnv_map.len;
    break;
}   

filename=QFileDialog::getSaveFileName(this,"Saved file name",filename,"All files (*.*)");
if (filename.isEmpty()) return;

QFile out(filename,this);
if (!out.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(0,"Error","File creation error");
    return;
}
out.write((char*)(data+start),len);
out.close();
}


//********************************************************************
//* Slots for extracting component images
//********************************************************************
void nvdedit::extract1() { extractor(0); }
void nvdedit::extract2() { extractor(1); }
void nvdedit::extract3() { extractor(2); }
void nvdedit::extract4() { extractor(3); }


//********************************************************************
//* Replacing components
//*   0 - NVIMG
//*   1 - Base XML
//*   2 - Custom XML
//*   3 - XNV MAP
//********************************************************************
void nvdedit::replacer(int type) {

QString filename="";
uint32_t fsize;

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
uint8_t* fbuf=new uint8_t[fsize]; // file buffer
bzero(fbuf,fsize);
out.read((char*)fbuf,fsize);
out.close();

// set the pointer to the file buffer, discard the old data
switch(type) {
  case 0:
    delete nvpart;
    nvpart=fbuf;
    hdr.nv_bin.len=fsize;
    break;
    
  case 1:
    delete xmlpart;
    xmlpart=fbuf;
    hdr.xnv_xml.len=fsize;
    break;
    
  case 2:
    delete custxmlpart;
    custxmlpart=fbuf;
    hdr.cust_xml.len=fsize;
    break;
    
  case 3:
    delete xmlmap;
    xmlmap=fbuf;
    hdr.xnv_map.len=fsize;
    break;
}   
// Recreate the data area
rebuild_data();

}

//********************************************************************
//* Slots for replacing component images
//********************************************************************
void nvdedit::replace1() { replacer(0); }
void nvdedit::replace2() { replacer(1); }
void nvdedit::replace3() { replacer(2); }
void nvdedit::replace4() { replacer(3); }


//********************************************************************
//* Slot for editing the binary NV database
//********************************************************************
void nvdedit::nvexpl() {

nvexplorer* exp=new nvexplorer(data+hdr.nv_bin.off,hdr.nv_bin.len);
exp->show();
}
  

//********************************************************************
//* XML component editor
//********************************************************************
void nvdedit::xeditor(int pn) {
 
viewer* viewpanel; 
  
switch(pn) {
  case 1:
    viewpanel=new viewer(xmlpart,&hdr.xnv_xml.len,0,"Base XML component");
    break;

  case 2:  
    viewpanel=new viewer(custxmlpart,&hdr.cust_xml.len,0,"Base XML component");
    break;

  default:
    viewpanel=0;
    return;
}

connect(viewpanel,SIGNAL(changed()),this,SLOT(setchanged()));
}  


//********************************************************************
//* Slots for editing XML components
//********************************************************************
void nvdedit::xedit2() { xeditor(1); }
void nvdedit::xedit3() { xeditor(2); }



//********************************************************************
//* Rebuilding the data area
//********************************************************************
void nvdedit::rebuild_data() {

uint32_t off;
uint32_t hdsize;
uint32_t totalsize;
uint8_t* newdata;

// header size
if (filetype == 1) hdsize=7*sizeof(struct nv_file_info);
else hdsize=sizeof(nv_dload_packet_head);
  
// Calculate the new partition size
totalsize=hdr.nv_bin.len+hdr.xnv_xml.len+hdr.cust_xml.len+hdr.xnv_map.len;

// Allocate memory for a new partition image (images of parts + header + 4 bytes of checksum)
newdata=new uint8_t[hdsize+totalsize+4];

// set up the source-receiver pointers
off=hdsize;

// copy partitions

if (hdr.nv_bin.len != 0) {
 hdr.nv_bin.off=off;
 memcpy(newdata+off,nvpart,hdr.nv_bin.len);
 off+=hdr.nv_bin.len;
}

if (hdr.xnv_xml.len != 0) {
  hdr.xnv_xml.off=off;
  memcpy(newdata+off,xmlpart,hdr.xnv_xml.len);
  off+=hdr.xnv_xml.len;
}  

if (hdr.cust_xml.len != 0) {
  hdr.cust_xml.off=off;
  memcpy(newdata+off,custxmlpart,hdr.cust_xml.len);
  off+=hdr.cust_xml.len;
}  

if (hdr.xnv_map.len != 0) {
  hdr.xnv_map.off=off;
  memcpy(newdata+off,xmlmap,hdr.xnv_map.len);
//   off+=hdr.xnv_map.len;
}  
// copy the header
memcpy(newdata,&hdr,hdsize);

// substitute the new size for the old one
plen=totalsize+hdsize+4;

// Copy the old CS. I don't know how to calculate it yet, and it's not needed
memcpy(newdata+plen-4,data+plen-4,4);

// Substitute the new data buffer for the old one 
delete data;
data=newdata;
}

