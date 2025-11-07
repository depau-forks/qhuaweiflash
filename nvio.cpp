//------------------- Library for working with the structure of binary nv-files --------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "nvexplorer.h"

// Maximum allowed cell size
#define max_item_len 10000

//******************************************************
// Getting the offset to the beginning of the file by file number
//******************************************************
uint32_t nvexplorer::fileoff(int fid) {

int i;
for (i=0;i<(int)nvhd.file_num;i++) {
  if (flist[i].id == (uint32_t)fid) return flist[i].offset;
}
printf("\n - File structure error - component #%i does not exist\n",fid);
exit(1);
}

//******************************************************
// Getting the index by file number
//******************************************************
int32_t nvexplorer::fileidx(int fid) {

int i;

for (i=0;i<(int)nvhd.file_num;i++) {
  if (flist[i].id == (uint32_t)fid) return i;
}
return -1;
}


//******************************************************
// Getting the offset to the beginning of the cell by its index
//******************************************************
uint32_t nvexplorer::itemoff_idx(int idx) {

return itemlist[idx].off+fileoff(itemlist[idx].file_id);
}


//******************************************************
//* Getting the cell index by its id
//*  return -1 - cell not found
//******************************************************
int32_t nvexplorer::itemidx(int item) {
  
int i;

for(i=0;i<(int)nvhd.item_count;i++) {
  if (itemlist[i].id == item) return i;
}
return -1;
}

//******************************************************
// Getting the offset to the beginning of the cell by its number
//******************************************************
int32_t nvexplorer::itemoff (int item) {

int idx=itemidx(item);
if (item == -1) return -1;
return itemoff_idx(idx);
}

//******************************************************
// Getting the cell size by its number
//******************************************************
int32_t nvexplorer::itemlen (int item) {

int idx=itemidx(item);
if (idx == -1) return -1;
return itemlist[idx].len;

}

//**********************************************
//*  Find the minimum of 2 numbers
//**********************************************
int min(int a, int b) {
  
if (a<b) return a;
else return b;
}

//**********************************************
//* Loading a cell into a buffer
//**********************************************
int nvexplorer::load_item(int item, char* buf) {
  
int idx=itemidx(item);
int len=itemlist[idx].len;

if (idx == -1) return -1; // not found
memcpy(buf,pdata+itemoff_idx(idx),len);
return len;
}

