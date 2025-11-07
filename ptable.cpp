//Procedures for working with the partition table

#include <QtWidgets>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
#include "ptable.h"
#include "sio.h"
#include "signver.h"

int dload_id=-1;
void fdirlist(int np);

void set_modified();

//******************************************************
//*  search for the symbolic name of a partition by its code
//******************************************************

void  find_pname(unsigned int id,unsigned char* pname, enum parttypes* ptype) {

unsigned int j;
struct  pcl{
  char name[20];
  uint32_t code;
  enum parttypes type;
} pcodes[]={ 
//  --- name ---       Part ID     type    
  {"M3Boot",          0x20000    ,part_bin}, 
  {"Ptable"          ,0x10000    ,part_ptable}, 
  {"M3Boot_R11"      ,0x200000   ,part_bin}, 
  {"Ptable_ext_A"    ,0x480000   ,part_ptable},
  {"Ptable_ext_B"    ,0x490000   ,part_ptable},
  {"Fastboot"        ,0x110000   ,part_bin},
  {"Logo"            ,0x130000   ,part_bin},
  {"Kernel"          ,0x30000    ,part_bin},
  {"Kernel_R11"      ,0x90000    ,part_bin},
  {"DTS_R11"         ,0x270000   ,part_bin},
  {"VxWorks"         ,0x40000    ,part_bin},
  {"VxWorks_R11"     ,0x220000   ,part_bin},
  {"M3Image"         ,0x50000    ,part_bin},
  {"M3Image_R11"     ,0x230000   ,part_bin},
  {"DSP"             ,0x60000    ,part_bin},
  {"DSP_R11"         ,0x240000   ,part_bin},
  {"Nvdload"         ,0x70000    ,part_nvram},
  {"Nvdload_R11"     ,0x250000   ,part_nvram},
  {"Nvimg"           ,0x80000    ,part_nvram},
  {"System"          ,0x590000   ,part_cpio},
  {"System"          ,0x100000   ,part_cpio},
  {"APP"             ,0x570000   ,part_cpio}, 
  {"APP"             ,0x5a0000   ,part_cpio}, 
  {"APP_EXT_A"       ,0x450000   ,part_cpio}, 
  {"APP_EXT_B"       ,0x460000   ,part_cpio},
  {"CDROMISO"        ,0xb0000    ,part_iso},
  {"Oeminfo"         ,0xa0000    ,part_oem},
  {"Oeminfo"         ,0x550000   ,part_oem},
  {"Oeminfo"         ,0x510000   ,part_oem},
  {"Oeminfo"         ,0x1a0000   ,part_oem},
  {"WEBUI"           ,0x560000   ,part_cpio},
  {"WEBUI"           ,0x5b0000   ,part_cpio},
  {"Wimaxcfg"        ,0x170000   ,part_bin},
  {"Wimaxcrf"        ,0x180000   ,part_bin},
  {"Userdata"        ,0x190000   ,part_cpio},
  {"Online"          ,0x1b0000   ,part_cpio},
  {"Online"          ,0x5d0000   ,part_cpio},
  {"Online"          ,0x5e0000   ,part_cpio},
  {"Ptable_R1"       ,0x100      ,part_ptable},
  {"Bootloader_R1"   ,0x101      ,part_bin},
  {"Bootrom_R1"      ,0x102      ,part_bin},
  {"VxWorks_R1"      ,0x550103   ,part_bin},
  {"Fastboot_R1"     ,0x104      ,part_bin},
  {"Kernel_R1"       ,0x105      ,part_bin},
  {"System_R1"       ,0x107      ,part_bin},
  {"Nvimage_R1"      ,0x66       ,part_nvram},
  {"WEBUI_R1"        ,0x113      ,part_bin},
  {"APP_R1"          ,0x109      ,part_bin},
  {"HIFI_R11"        ,0x280000   ,part_bin},
  {"Firmwares"       ,0x1e0000   ,part_bin},
  {"Teeos"	     ,0x290000   ,part_bin},
  {0,0}
};

for(j=0;pcodes[j].code != 0;j++) {
  if(pcodes[j].code == id) {
    break;
  }  
}
if (pcodes[j].code != 0) { 
    strcpy((char*)pname,pcodes[j].name); // name found - copy it to the structure
    *ptype=pcodes[j].type;
}    
else {
    sprintf((char*)pname,"U%08x",id); // name not found - substitute the pseudo-name Uxxxxxxxx in a blunt-ended format
    *ptype=part_bin;
}    
}



//****************************************************
//* Getting a description of the firmware type by code
//****************************************************
char* fw_description(uint8_t code) {
  
// table of signature types
char* fwtypes[]={
"00-UNKNOWN",        // 0
"01-ONLY_FW",        // 1
"02-ONLY_ISO",       // 2
"03-FW_ISO",         // 3
"04-ONLY_WEBUI",     // 4
"05-FW_WEBUI",       // 5
"06-ISO_WEBUI",      // 6
"07-FW_ISO_WEBUI"    // 7
};  

return fwtypes[code&0x7];  
}

//*******************************************************************
//* Extracting a partition from a file and adding it to the partition table
//*
//  in - input firmware file
//  The position in the file corresponds to the beginning of the partition header
//*******************************************************************
void ptable_list::extract(FILE* in)  {

uint16_t hcrc,crc;
QString str;
uint16_t* crcblock;
uint32_t crcblocksize;
uint8_t* zbuf;
long unsigned int zlen;
int res;

// read the header into the structure
fread(&table[npart].hd,1,sizeof(pheader),in); // header

//  Search for the symbolic name of the partition in the table 
find_pname(code(npart),table[npart].pname,&table[npart].ptype);

// load the checksum block
table[npart].csumblock=0;  // until the block is created
crcblock=(uint16_t*)malloc(crcsize(npart)); // allocate temporary memory for the loaded block
crcblocksize=crcsize(npart);
fread(crcblock,1,crcblocksize,in);

// load the partition image
table[npart].pimage=(uint8_t*)malloc(psize(npart));
fread(table[npart].pimage,1,psize(npart),in);

// check the header CRC
hcrc=table[npart].hd.crc;
table[npart].hd.crc=0;  // the old CRC is not taken into account in the calculation
crc=crc16((uint8_t*)&table[npart].hd,sizeof(pheader));
if (crc != hcrc) {
    str.sprintf("Partition %s (%02x) - header checksum error",table[npart].pname,code(npart)>>16);
    QMessageBox::warning(0,"CRC Error",str);
}  
table[npart].hd.crc=crc;  // restore CRC

// calculate and check the partition CRC
calc_crc16(npart);
if (crcblocksize != crcsize(npart)) {
    str.sprintf("Partition %s (%02x) - incorrect checksum block size",table[npart].pname,code(npart)>>16);
    QMessageBox::warning(0,"CRC Error",str);
}  
  
else if (memcmp(crcblock,table[npart].csumblock,crcblocksize) != 0) {
    str.sprintf("Partition %s (%02x) - incorrect block checksum",table[npart].pname,code(npart)>>16);
    QMessageBox::warning(0,"CRC Error",str);
}  
  
free(crcblock);

// Definition of zlib-compression

table[npart].zflag=0; 

if ((*(uint16_t*)table[npart].pimage) == 0xda78) {
  table[npart].zflag=table[npart].hd.psize;  // save the compressed size 
  zlen=52428800;
  zbuf=(uint8_t*)malloc(zlen);  // buffer in 50M
  // unpack the partition image
  res=uncompress (zbuf, &zlen, table[npart].pimage, table[npart].hd.psize);
  if (res != Z_OK) {
    printf("\n! Error unpacking partition %s (%02x)\n",table[npart].pname,table[npart].hd.code>>16);
    exit(0);
  }
  // create a new partition image buffer and copy the unpacked data into it
  free(table[npart].pimage);
  table[npart].pimage=(uint8_t*)malloc(zlen);
  memcpy(table[npart].pimage,zbuf,zlen);
  table[npart].hd.psize=zlen;
  free(zbuf);
  // recalculate checksums
  calc_crc16(npart);
//   table[npart].hd.crc=crc16((uint8_t*)&table[npart].hd,sizeof(struct pheader));
}

// advance the partition counter
npart++;
// move forward to the word boundary if necessary
res=ftell(in);
if ((res&3) != 0) fseek(in,(res+4)&(~3),SEEK_SET);
}

//*******************************************************
//* Clearing the partition table
//*******************************************************
void ptable_list::clear() {

int i;
for (i=0;i<npart;i++) {
  free(table[i].csumblock);
  free(table[i].pimage);
}
npart=0;
}


//*******************************************************
//*  Search for partitions in the firmware file
//* 
//* returns the number of partitions found
//*******************************************************
void ptable_list::findparts(FILE* in) {


const unsigned int dpattern=0xa55aaa55; // Partition header start marker   
unsigned int i;
uint8_t percent,oldpercent=0;
uint32_t filesize;

// BIN-file prefix buffer
uint8_t prefix[0x5c];

// Create a progress bar dialog panel
QWidget* pb=new QWidget();
QVBoxLayout* lm=new QVBoxLayout(pb);

QLabel* label = new QLabel("Search and load partitions",pb);
QFont font;
font.setPointSize(14);
font.setBold(true);
font.setWeight(75);
label->setFont(font);
lm->addWidget(label);

QProgressBar* fbar = new QProgressBar(pb);
fbar->setValue(0);
lm->addWidget(fbar);

pb->show();

// get the file size
fseek(in,0,SEEK_END);
filesize=ftell(in);
rewind(in);

// search for the beginning of the partition chain in the file
while (fread(&i,1,4,in) == 4) {
  // indicator update
  percent=ftell(in)*100/filesize;
  if (percent>oldpercent) {
   fbar->setValue(percent);
   QCoreApplication::processEvents();
   oldpercent=percent;
  } 

  if (i == dpattern) break; // marker found
}
if (feof(in)) {
  QMessageBox::critical(0,"Error"," No partitions found in the file - the file does not contain a firmware image");
    exit(0);
}  

// the current position in the file should not be closer than 0x60 from the beginning - the size of the header of the entire file
if (ftell(in)<0x60) {
    QMessageBox::critical(0,"Error","The file header has the wrong size");
    exit(0);
}    
fseek(in,-0x60,SEEK_CUR); // move to the beginning of the BIN-file
// extract the prefix
fread(prefix,0x5c,1,in);
if (dload_id == -1) {
  // For the first loaded file, we extract the firmware type
  dload_id=prefix[0];
  // if dload_id is not set forcibly - select it from the header
  if (dload_id > 0xf) {
    QMessageBox::critical(0,"Error","Invalid firmware type code (dload_id) in the header");
    printf("\n Invalid firmware type code (dload_id) in the header - %x",dload_id);
    exit(0);
  }
  dload_id&=7; // remove the signature presence bit
  printf("\n Firmware file code: %x (%s)",dload_id,fw_description(dload_id));
}

// Search for partitions
do {
  // indicator update
  percent=ftell(in)*100/filesize;
  if (percent>oldpercent) {
   fbar->setValue(percent);
   QCoreApplication::processEvents();
   oldpercent=percent;
  } 
  if (fread(&i,1,4,in) != 4) break; // end of file
  if (i != dpattern) break;         // sample not found - end of partition chain
  fseek(in,-4,SEEK_CUR);            // move back to the beginning of the header
  extract(in);                      // extract the partition
} while(1);

delete fbar;
delete lm;
delete label;
delete pb;  
}

//*******************************************************
//*  Replacing the partition image with the contents of the file 
//*******************************************************
void ptable_list::loadimage(int np, FILE* in) {

uint32_t fsize;

// determine the file size
fseek(in,0,SEEK_END);
fsize=ftell(in);
rewind(in);

// allocate memory for a new partition
free(table[np].pimage);
table[np].pimage=(uint8_t*)malloc(fsize);

// read the new partition image
fread(table[np].pimage,1,fsize,in);

// correct the partition size in the header
table[np].hd.psize=fsize;
// recalculate block crc16
calc_crc16(np);
fclose(in);
// set the data modification flag
set_modified();
}

//*******************************************************
//* Writing a full partition image to a file
//*******************************************************
void ptable_list::save_part(int np,FILE* out,bool zflag) {
 
uint32_t pos,i,cnt;
uint8_t pad=0;  
long unsigned int clen;


struct ptb_t origpt=table[np]; // save the old partition descriptor
if (zflag) {
  // compressing the partition image
  table[np].pimage=(uint8_t*)malloc(table[np].hd.psize+64000);
  clen=table[np].hd.psize+64000;
  compress2(table[np].pimage,&clen,origpt.pimage,origpt.hd.psize,9); 
  table[np].hd.psize=clen;
  calc_crc16(np);
}  
fwrite(hptr(np),1,sizeof(pheader),out);   // header
fwrite(table[np].csumblock,1,crcsize(np),out);  // crc
fwrite(iptr(np),1,psize(np),out);   // body
// Aligning the tail to the word boundary
pos=ftell(out);
if ((pos&3) != 0) {
  cnt=4-(pos%4); // get the number of extra bytes;
  for(i=0;i<cnt;i++) fwrite(&pad,1,1,out);  // write zeros to the word boundary
}  
if (zflag) {
  // clean the buffers
  free(table[np].pimage);
  free(table[np].csumblock);
  table[np]=origpt;
  table[np].csumblock=0;
  calc_crc16(np);
}  

}

//*******************************************************
//*  Calculating the block checksum of the header
//*******************************************************
void ptable_list::calc_hd_crc16(int n) { 

table[n].hd.crc=0;   
table[n].hd.crc=crc16((uint8_t*)hptr(n),sizeof(pheader));   
}


//*******************************************************
//*  Calculating the block checksum of the partition 
//*******************************************************
void ptable_list::calc_crc16(int n) {
  
uint32_t csize; // size of the sum block in 16-bit words
uint16_t* csblock;  // pointer to the created block
uint32_t off,len;
uint32_t i;
uint32_t blocksize=table[n].hd.blocksize; // size of the block covered by the sum

// determine the size and create a block
csize=psize(n)/blocksize;
if (psize(n)%blocksize != 0) csize++; // This is if the image size is not a multiple of blocksize
csblock=(uint16_t*)malloc(csize*2);

// sum calculation cycle
for (i=0;i<csize;i++) {
 off=i*blocksize; // offset to the current block 
 len=blocksize;
 if ((psize(n)-off)<blocksize) len=psize(n)-off; // for the last incomplete block 
 csblock[i]=crc16(iptr(n)+off,len);
} 
// write the parameters in the header
if (table[n].csumblock != 0) free(table[n].csumblock); // destroy the old block, if it was
table[n].csumblock=csblock;
table[n].hd.hdsize=csize*2+sizeof(pheader);
// recalculate the header CRC
calc_hd_crc16(n);
  
}

  
//*******************************************************
//* Deleting a partition
//*******************************************************
void ptable_list::delpart(int n) {

int i;
// free the occupied memory
free(table[n].csumblock);
free(table[n].pimage);
// move the partition chain up
for (i=n;i<index()-1;i++)  table[i]=table[i+1];
// decrease the partition counter
npart--;
// set the data modification flag
set_modified();
}  
  

//*******************************************************
//* Moving a partition up
//*******************************************************
void ptable_list::moveup(int n) {

struct ptb_t tmp;

if (n == 0) return;
tmp=table[n-1];
table[n-1]=table[n];
table[n]=tmp;
// set the data modification flag
set_modified();
}

//*******************************************************
//* Moving a partition down
//*******************************************************
void ptable_list::movedown(int n) {

struct ptb_t tmp;

if (n == (npart-1)) return;
tmp=table[n+1];
table[n+1]=table[n];
table[n]=tmp;
// set the data modification flag
set_modified();
}

//*******************************************************
//* Replacing the partition image
//*******************************************************
void ptable_list::replace(int n, uint8_t* data, uint32_t len) {

free (table[n].pimage);
table[n].pimage=(uint8_t*)malloc(len);
memcpy(table[n].pimage,data,len);
table[n].hd.psize=len;
calc_crc16(n);
// set the data modification flag
set_modified();
}



