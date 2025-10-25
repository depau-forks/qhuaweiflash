#ifndef __PTABLE_H
#define __PTABLE_H

// partition header description structure
struct __attribute__ ((__packed__)) pheader {
 uint32_t magic;    //   0xa55aaa55
 uint32_t hdsize;   // header size
 uint32_t hdversion; // header version
 uint8_t unlock[8]; // platform
 uint32_t code;     // partition type
 uint32_t psize;    // data field size
 uint8_t date[16];
 uint8_t time[16];  // firmware build date-time
 uint8_t version[32];   // firmware version
 uint16_t crc;   // header CRC
 uint32_t blocksize;  // firmware image CRC block size
}; 

// Partition structure types
enum parttypes {
    part_bin, // unformatted binary partitions
    part_cpio,   // CPIO format partitions
    part_nvram,  // nvdload partitions
    part_iso,    // CD images
    part_ptable, // partition tables
    part_oem     // oeminfo 
};    

// Partition table description structure

struct ptb_t{
  unsigned char pname[20];    // partition text name
  struct pheader hd;  // header image
  uint16_t* csumblock; // checksum block
  uint8_t* pimage;   // partition image
  uint32_t zflag;     // compressed partition flag  
  enum parttypes ptype;     // partition type, according to enum parttypes
};

//**********************************************************
//*  Class for working with partition table
//**********************************************************

class ptable_list {
  // partition table storage
  struct ptb_t table[120];
  int npart; // number of partitions in table
  
public:
  // constructor
  ptable_list() { npart=0; }
  // destructor
  ~ptable_list() {clear();}
  // extract partitions from file 
  void extract(FILE* in);  
  // clear entire table
  void clear();
  // get table size
  int index() {return npart; }
  // get header size
  uint32_t crcsize(int n) { return table[n].hd.hdsize-sizeof(pheader); }
  // get image size
  uint32_t psize(int n) { return table[n].hd.psize; }
  // get partition code
  uint32_t code(int n) { return table[n].hd.code; }
  // get partition name
  uint8_t* name(int n) { return table[n].pname; }
  // get header reference
  struct pheader* hptr(int n) { return &table[n].hd; }
  // get partition image reference
  uint8_t* iptr(int n) { return table[n].pimage; }
  // get partition type
  enum parttypes ptype(int n) { return table[n].ptype; }
  // get compressed size
  uint32_t zsize(int n) { return table[n].zflag; }
  
  // get references to header descriptive fields
  uint8_t* platform(int n) { return table[n].hd.unlock; }
  uint8_t* date(int n) { return table[n].hd.date; }
  uint8_t* time(int n) { return table[n].hd.time; }
  uint8_t* version(int n) { return table[n].hd.version; }

  // replace partition image
  void replace(int n, uint8_t* data, uint32_t len);
  
  void findparts(FILE* in);
  void loadimage(int np, FILE* in);
  void save_part(int n,FILE* out, bool zflag);
  void calc_crc16(int n);
  void calc_hd_crc16(int n); 

  // delete partition
  void delpart(int n);
  // move up
  void moveup(int n);
  // move down
  void movedown(int n);  
};
    

extern ptable_list* ptable;  

char* fw_description(uint8_t code);

void  find_pname(unsigned int id,unsigned char* pname);

// firmware type
extern int dload_id;

#endif
