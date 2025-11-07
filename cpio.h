// 
//  cpio file editor
// 
#ifndef _CPIO_H
#define _CPIO_H

#include <QtWidgets>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "ptable.h"
#include "viewer.h"
#include "hexfileviewer.h"
#include "cpfiledir.h"

//*****************************************************
//* cpio partition editor class
//*****************************************************
class cpioedit: public QWidget {
  
Q_OBJECT

int pnum;
// pointers to the partition image
uint8_t* pdata;
uint32_t plen;

QToolBar* toolbar;
QTableWidget* cpiotable=0;
QVBoxLayout* vlm;

viewer* view; // file view window
hexfileviewer* hview; // hex editor window

QList<cpfiledir*>* rootdir=0;   // pointer to the root partition vector
QList<cpfiledir*>* currentdir;  // current directory vector
void cpio_hide_dir();
int current_file_index();
cpfiledir* selected_file();
void cpio_show_dir(QList<cpfiledir*>* dir, int focusmode);
void fileeditor(bool readonly);
void repack_cpio();

// partition change flag
bool is_modified=false;

QMenuBar* menubar;
QMenu* menu_edit;

public:
cpioedit(int xpnum,QMenuBar* mbar, QWidget* parent); 
~cpioedit();


public slots:
void cpio_process_file(int, int); // file selection processing
void extract_file();  // file extractor
void replace_file();  // file replacement
void delete_file();  // deleting files
void view_file();   // view
void edit_file();   // view
void add_file();    // add new file
void add_dir();    // create directory
void hexedit_file();   // hex-viewer/editor
void setModified() {is_modified=true;}  // setting the archive content modification flag
void saveall();
void menuenabler();
void go_up();
};


#endif
