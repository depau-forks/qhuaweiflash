// nvdload partition editor 

#ifndef __NVDEDIT_H_H
#define __NVDEDIT_H_H
#include <stdint.h>
#include <QtWidgets>

// Descriptors of the nvdload partition header format
// Taken from the Huawei kernel sources

#define NV_FILE_MAGIC 0x766e  // nv

// Descriptor of each component 
struct nv_file_info {
    uint32_t magic;      // signature 0x766e(nv)
    uint32_t off;            /*file offset in one section*/
    uint32_t len;            /*file lenght */
};

// nvdload partition header 
// for old chipsets (up to V7R11 inclusive) the ulSimNumEx and trash fields are missing
struct nv_dload_packet_head {

    struct nv_file_info nv_bin;      // nvimg image
    struct nv_file_info xnv_xml;  // Main XML component
    struct nv_file_info xnv_xml2; 
    struct nv_file_info cust_xml; // Additional XML component
    struct nv_file_info cust_xml2; 
    struct nv_file_info xnv_map;  
    struct nv_file_info xnv_map2;  
    uint32_t ulSimNumEx;                  // Number of supported modems minus 2
    uint8_t trash[36]; // additional modem descriptors, not used here
//     STRU_XNV_MAP_FILE_INFO xnv_file[0]; 
};

//***********************************************************
//* Editor main window class
//***********************************************************
class nvdedit  : public QWidget {

Q_OBJECT

// Header
struct nv_dload_packet_head hdr;

bool changed=false;

QVBoxLayout* vlm;
QGridLayout* lcomp;
QSpacerItem* rspacer;

QLabel* hdrlabel;
QLabel* ntype;

QLabel* comphdr1;
QLabel* comphdr2;
QLabel* comphdr3;

QLabel* name1;
QLabel* name2;
QLabel* name3;
QLabel* name4;

QLabel* size1;
QLabel* size2;
QLabel* size3;
QLabel* size4;

QPushButton* extr1;
QPushButton* extr2;
QPushButton* extr3;
QPushButton* extr4;

QPushButton* repl1;
QPushButton* repl2;
QPushButton* repl3;
QPushButton* repl4;

QPushButton* edit1;
QPushButton* edit2;
QPushButton* edit3;


// number of this partition in the partition table
int pnum;

// local copy of the partition image
uint8_t* data;
uint32_t plen;// length-128, without huawei header

// file type
int filetype;

// copies of components
uint8_t* nvpart;
uint8_t* xmlpart=0;
uint8_t* custxmlpart=0;
uint8_t* xmlmap=0;

void rebuild_data();
void extractor(int type);
void replacer(int type);
void xeditor(int type);

public:

nvdedit(int xpnum, QWidget* parent);
~nvdedit();

public slots:

void extract1();
void extract2();
void extract3();
void extract4();
  
void replace1();
void replace2();
void replace3();
void replace4();

void nvexpl();
void xedit2();
void xedit3();

//* Slot for setting the change flag
void setchanged() {changed=true;}
};


#endif
 