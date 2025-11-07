//-------------- NVRAM binary image editor ----------------------------
#ifndef _NVEXPLORER_H_
#define _NVEXPLORER_H_
#include <stdint.h>
#include <QtWidgets>




//------------------------- nvram file data structures ------------------------------
// Huawei data types
#define U32 uint32_t 
#define U16 uint16_t
#define U8 uint8_t

#define FILE_MAGIC_NUM 0x224e4944 // File header signature

//  NVRAM file structure
// 
//------- control structure -----------------
// +00 Header (nvfile_header) - 96 bytes
// +file_offset - file catalog
// +item_offset - cell catalog
// 4 bytes of CRC of the control structure
//-------- Cell data ----------------------
// data goes sequentially cell by cell, without gaps
//


// nv-file header structure
struct nvfile_header {

    U32 magicnum;   // signature
    U32 ctrl_size;  // size of control structures (offset to data)
    U16 version;    // * file version * /
    U8 modem_num;   // modem number for multi-modem configurations
    U8 crcflag;     // CRC presence flag
    U32 file_offset; // offset to the file list
    U32 file_num;    // number of files in the list 
    U32 file_size;   // file list size
    U32 item_offset;  // offset to the cell list
    U32 item_count;   // number of cells in the list
    U32 item_size;    // cell list size
    U8 reserve2 [12];
    U32 timetag [4]; // time stamp
    U8 product_version [32]; // device version
};

//  File catalog element
struct nv_file {
    U32 id; // file number
    U8 name [28]; // file name
    U32 size; // file size
    U32 offset; // offset to the file
};

// Cell catalog element
struct nv_item {
    U16 id; // cell number
    U16 len; // size in bytes
    U32 off; // offset from the beginning of the file
    U16 file_id; // the file to which the cell belongs
    U16 priority; // cell priority
    U8 modem_num; // modem number
    U8 reserved [3]; 
};


//***********************************************************
//* Editor main window class
//***********************************************************
class nvexplorer  : public QMainWindow {

Q_OBJECT

bool changed=false;

uint8_t* srcdata;
uint8_t* pdata;
uint32_t plen;

struct nvfile_header nvhd; // nvram-file header
// File catalog
struct nv_file flist[15];
// cell catalog
struct nv_item* itemlist;
// uint32_t maxitemlen=0; // maximum cell size

// nvio library subroutines for accessing nvram structures
uint32_t fileoff(int fid);
int32_t fileidx(int fid);
uint32_t itemoff_idx(int idx);
int32_t itemidx(int item);
int32_t itemoff (int item);
int32_t itemlen (int item);
int load_item(int item, char* buf);
void datacell(int);
void changed_item(int);

// CRC type used in the file
// 0 - no crc
// 1 - the first type of CRC, for V7R11, with an array of checksums
// 2 - the second type of CRC, for V7R22, with an individual CS
int crcmode;
// Offset to the CRC field in the nvram image
uint32_t crcoff;
// Subroutines for working with CRC
uint32_t calc_crcsize();
void recalc_crc();
void recalc_ctrl_crc();
uint32_t load_item_crc(int);
uint32_t calc_item_crc(int);
void restore_item_crc(int);
bool verify_item_crc(int idx);

// GUI
QWidget* central;
QSettings* config;
QVBoxLayout* vlm;

// nvram table
QTableWidget* nvtable;

// Main menu
QMenuBar* menubar;
QMenu* menu_file;
QMenu* menu_edit;
QMenu* menu_view;

// toolbar
QToolBar* toolbar;

void zoom(int);

public:
nvexplorer(uint8_t* data, uint32_t len);
~nvexplorer();

public slots:
void save_all();  
void zoomin();
void zoomout();
void edititem();  
void extract_item();
void replace_item();

};

char* find_desc(int item);

#endif // _NVEXPLORER_H_