// 
//  cpfiledir - class for storing a list of files that make up a cpio archive
// 
#include <QtCore/QVariant>
#include <QtWidgets>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include "cpio.h"

//********************************************************************
//* File storage class constructor - adding from cpio stream
//********************************************************************
cpfiledir::cpfiledir(uint8_t* iptr) {
  
phdr=new cpio_header_t;

memcpy(phdr,iptr,sizeof(cpio_header_t)); // copy the header to ourselves
int nsz=nsize();
volatile int fsz=fsize();

// file name
filename=new char[nsz];
memcpy(filename,iptr+sizeof(cpio_header_t),nsz);

// file body
if (fsz != 0) fimage=new char[fsz];
memcpy(fimage,iptr+sizeof(cpio_header_t)+nsz,fsz);

}

//********************************************************************
//* Constructor for adding files from the outside
//********************************************************************
cpfiledir::cpfiledir(cpio_header_t* header, uint8_t* fname, uint8_t* data) {

phdr=new cpio_header_t;

memcpy(phdr,header,sizeof(cpio_header_t)); // copy the header to ourselves
int nsz=nsize();
volatile int fsz=fsize();

// file name
filename=new char[nsz];
memcpy(filename,fname,nsz);

// file body
if (fsz != 0) fimage=new char[fsz];
memcpy(fimage,data,fsz);
}
    

//******************************************************
//* File storage class destructor
//******************************************************
cpfiledir::~cpfiledir() {

if ((subdir != 0) && !updirflag) {  // if this is not a link to the parent directory
    // delete the subdirectory vector with all its contents
    qDeleteAll(*subdir);
    subdir->clear();
    delete subdir;
}  

delete [] filename;
if (fimage != 0) delete [] fimage;
delete phdr;

}

//*******************************************************
//* Set new file name
//*******************************************************
void cpfiledir::setfname (char* name) {
  
int len;

delete [] filename;
len=strlen(name)+1;
filename=new char[len];
strcpy(filename,name);
setfsize(len);
  
}

//*******************************************************
//* Get file size
//*******************************************************
 uint32_t cpfiledir:: fsize() {
  
uint32_t val;
char vstr[9];
bzero(vstr,9);
strncpy(vstr,phdr->c_filesize,8);
val=strtoul(vstr,0,16);
return val;
}

//*******************************************************
//* Set file size
//*******************************************************
void cpfiledir::setfsize(int size) {
  
char str[10];  

sprintf(str,"%08x",size);  
memcpy(phdr->c_filesize,str,8);

}



//*******************************************************
//* Get rounded file name length
//*******************************************************
uint32_t cpfiledir:: nsize() {
  
uint32_t val;
char vstr[9];

bzero(vstr,9);
strncpy(vstr,phdr->c_namesize,8);
val=strtoul(vstr,0,16);
val+=sizeof(cpio_header_t); // add header size
if ((val&3) != 0) val=(val&0xfffffffc)+4; // round up to 4 bytes
return val-sizeof(cpio_header_t);
}

//**********************************************************
//* Get clean file name without preceding path
//**********************************************************
char* cpfiledir::cfname() {
  
char* ptr;

ptr=strrchr(filename,'/');
if (ptr == 0) return filename;
else return ptr+1;
}

//*******************************************************
//* Get file creation time
//*******************************************************
uint32_t cpfiledir::ftime() {
  
uint32_t val;
char vstr[9];

bzero(vstr,9);
strncpy(vstr,phdr->c_mtime,8);
val=strtoul(vstr,0,16);
return val;
}


//*******************************************************
//* Get file attributes
//*******************************************************
uint32_t cpfiledir::fmode() {
  
uint32_t val;
char vstr[9];

bzero(vstr,9);
strncpy(vstr,phdr->c_mode,8);
val=strtoul(vstr,0,16);
return val;
}

//*******************************************************
//* Get file group
//*******************************************************
uint32_t cpfiledir::fgid() {
  
uint32_t val;
char vstr[9];

bzero(vstr,9);
strncpy(vstr,phdr->c_gid,8);
val=strtoul(vstr,0,16);
return val;
}

//*******************************************************
//* Get file owner
//*******************************************************
uint32_t cpfiledir::fuid() {
  
uint32_t val;
char vstr[9];

bzero(vstr,9);
strncpy(vstr,phdr->c_uid,8);
val=strtoul(vstr,0,16);
return val;
}

//*****************************************************************
//* Get the full size of a file or all files in a subdirectory
//*****************************************************************
uint32_t cpfiledir::treesize() {

uint32_t sum=0;
int i;

if (subdir == 0) return totalsize(); // for non-directories
for(i=1;i<subdir->count();i++) {
  sum+=subdir->at(i)->treesize();
}
return totalsize()+sum;
}

//*****************************************************************
//* Repacking the current directory into a cpio archive
//*  ptr - buffer for saving data
//*  returns the size of the received archive
//*****************************************************************
uint32_t cpfiledir::store_cpio(uint8_t* ptr) {

int i;
uint32_t len=sizeof(cpio_header_t);
uint32_t size,nlen;

// save the header
memcpy(ptr,phdr,len);
// file name
size=nsize();
memcpy(ptr+len,filename,size);
len+=size;
//file body
size=fsize();
memcpy(ptr+len,fimage,size);
len+=size;
// round up to 4 bytes
if ((len&3) != 0) {
  nlen=(len&0xfffffffc)+4;
  bzero(ptr+len,nlen-len);
  len=nlen;
}

// process subdirectories

if (subdir != 0) {
  for(i=1;i<subdir->count();i++) {
    len += subdir->at(i)->store_cpio(ptr+len);
  }
}

return len;
}

//*******************************************************
//* Replacing the file body
//*******************************************************
void cpfiledir::replace_data(uint8_t* pdata, uint32_t len) {

delete [] fimage;
fimage=new char[len];
memcpy(fimage,pdata,len);

setfsize(len);
}
  
  
//##############################################################################################################################################
  
//*******************************************************
//* Extracting the file name from the cpio archive header
//*******************************************************
void extract_filename(uint8_t* iptr, char* filename) {

strcpy(filename,(char*)(iptr+sizeof(cpio_header_t)));
}

//*******************************************************
//* Search for a subdirectory by name
//*******************************************************
QList<cpfiledir*>* find_dir(char* name, QList<cpfiledir*>* updir) {
  
int i;
char* fn;
for (i=0;i<updir->size();i++) {
  fn=updir->at(i)->cfname();
  if (strcmp(fn, name) == 0) return updir->at(i)->subdir;
}
return 0;
}

//*******************************************************
//* Search for a file by name in the specified directory
//*******************************************************
int find_file(QString name, QList<cpfiledir*>* dir) {

int i;
char* fn;
for (i=0;i<dir->size();i++) {
  fn=dir->at(i)->cfname();
  if (name == fn) return i;
}
return -1;
}
  
  
//*******************************************************
//* Determining the presence of a cpio stream
//*******************************************************
int is_cpio(uint8_t* ptr) {

if (strncmp((char*)ptr,"070701",6) == 0) {
  return 1;
}  
else {
  return 0;	
}
}

//*******************************************************
//* Loading a single file into a vector
//*
//* iptr - link to the file header in the cpio stream
//* dir - pointer to the directory to which the file belongs
//* plen - the total length of the memory area storing the archive
//* filename - file name without the preceding path.
//*******************************************************
uint32_t cpio_load_file(uint8_t* iptr, QList<cpfiledir*>* dir, int plen, char* fname) {

char* dfname=(char*)"..";  
QString str;
// class where the descriptors of this file are loaded
cpfiledir* fd;
char filename[256]; // buffer for a copy of the file name
char* slptr;
QList<cpfiledir*>* fdir; // subdirectory to search for the rest of the file name
strncpy(filename,fname,256);      

// Root directory
if ((strlen(filename) == 1) && (filename[1] != '.')) {
  fd=new cpfiledir(iptr);
  fd->subdir=0; // no subdirectory  
  dir->append(fd);
  return fd->totalsize();
}
// Check if the file path exists      
slptr=strchr(filename,'/');

if (slptr != 0) {
  // this is not yet the final file name, but a path element
  *slptr=0; // split the file name into the top directory and the rest
  slptr++;  // now slptr points to the rest of the file name
  fdir=find_dir(filename, dir); // look for a subdirectory in the current directory
  if (fdir == 0) {
    str.sprintf("File without a directory found in the stream - %s",fname);
    QMessageBox::critical(0,"CPIO Error",str);
    return 0; // not found - structure error, file without a directory
  }
// load the file into the subdirectory vector   
  return cpio_load_file(iptr,fdir,plen,slptr);
}  
// This is the real final file name
// for a directory, create a vector-subdirectory
fd=new cpfiledir(iptr);
if ((fd->fmode()&C_ISDIR) != 0) {
   // subdirectory vector
   fd->subdir=new QList<cpfiledir*>;
   // pointer to the upper-level directory (i.e. this one)
   cpfiledir* upfd=new cpfiledir(iptr);
   upfd->subdir=dir;
   // file name for it is ".."
   upfd->setfname(dfname);
   upfd->updirflag=true;  // sign of a link to the upper-level directory
   // add this entry first to the subdirectory vector
   fd->subdir->append(upfd);
}  
dir->append(fd);
return fd->totalsize();
}

//*******************************************************
//* Calculating the full size of the loaded archive
//*******************************************************
uint32_t fullsize(QList<cpfiledir*>* root) {
  
uint32_t sum=0;
int i;

for(i=0;i<root->count();i++) {
  sum+=root->at(i)->treesize();
}  
return sum;
}

