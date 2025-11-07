// 
//  Saving the firmware file to disk
// 
#ifndef _CPFILEDIR_H
#define _CPFILEDIR_H

#include <QtWidgets>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "ptable.h"

// #include "MainWindow.h"

// cpio-file attributes
#define C_IRUSR		000400
#define C_IWUSR		000200
#define C_IXUSR		000100
#define C_IRGRP		000040
#define C_IWGRP		000020
#define C_IXGRP		000010
#define C_IROTH		000004
#define C_IWOTH		000002
#define C_IXOTH		000001

#define C_ISUID		004000
#define C_ISGID		002000
#define C_ISVTX		001000

#define C_ISBLK		060000
#define C_ISCHR		020000
#define C_ISDIR		040000
#define C_ISFIFO	010000
#define C_ISSOCK	0140000
#define C_ISLNK		0120000
#define C_ISCTG		0110000
#define C_ISREG		0100000


//************************************************************
//* Archive element header
//************************************************************
struct __attribute__ ((__packed__)) cpio_header {
   char    c_magic[6];
   char    c_ino[8];
   char    c_mode[8];
   char    c_uid[8];
   char    c_gid[8];
   char    c_nlink[8];
   char    c_mtime[8];
   char    c_filesize[8];
   char    c_devmajor[8];
   char    c_devminor[8];
   char    c_rdevmajor[8];
   char    c_rdevminor[8];
   char    c_namesize[8];
   char    c_check[8];
};
typedef struct cpio_header cpio_header_t;

//*****************************************************
//* File system element storage class
//*****************************************************
class cpfiledir {

  cpio_header_t* phdr; // link to header
  char* filename; // pointer to file name
  char* fimage=0; // pointer to file body
    
public:
  cpfiledir(uint8_t* hdr);
  cpfiledir(cpio_header_t* header, uint8_t* fname, uint8_t* data);
  ~cpfiledir();
  QList<cpfiledir*>* subdir=0; // link to subdirectory container

//    char* fname() {return (char*)phdr+sizeof(cpio_header_t);} // link to file name
   char* fname() {return filename;} // link to file name
//    char* fdata() {return fname()+nsize();}  // link to file body
   char* fdata() {return fimage;}  // link to file body
   void setfdata(char* data) { fimage=data; }
   char* cfname(); // file name without path
   void setfname(char* name);
   
  uint32_t fsize(); // file size
  void setfsize(int size); // setting new file size
  uint32_t nsize(); // file name size
  uint32_t totalsize() { return sizeof(cpio_header_t)+nsize()+fsize();} // full size of the archive record about the file
  uint32_t fmode(); // file attribute flags
  uint32_t ftime();
  uint32_t fuid();
  uint32_t fgid();
  uint32_t treesize();
  uint32_t store_cpio(uint8_t* pdata);
  void replace_data(uint8_t* pdata, uint32_t len);
  bool updirflag=false; // indicator that this is a link to the parent directory 
};


int is_cpio(uint8_t* ptr);
void extract_filename(uint8_t* iptr, char* filename);
QList<cpfiledir*>* find_dir(char* name, QList<cpfiledir*>* updir);
int find_file(QString name, QList<cpfiledir*>* dir);
uint32_t cpio_load_file(uint8_t* iptr, QList<cpfiledir*>* dir, int plen, char* fname);
uint32_t fullsize(QList<cpfiledir*>* root);

#endif
