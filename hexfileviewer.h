// HEX-view and edit files from the cpio-archive vector

#ifndef __HEXFILEVIEWER_H
#define __HEXFILEVIEWER_H
#include <stdint.h>
#include <QtWidgets>
#include "cpfiledir.h"
#include "hexeditor.h"

//***********************************************************
//* Main window class 
//***********************************************************
class hexfileviewer  : public QMainWindow {

Q_OBJECT

cpfiledir* fileptr;
uint8_t* pdata;
uint32_t plen;

QWidget* central;
QSettings* config;
QVBoxLayout* vlm;
hexeditor* hed;

// Main menu
QMenuBar* menubar;
QMenu* menu_file;

// Status bar
QStatusBar* statusbar;

bool datachanged=false;

public:
  
hexfileviewer(cpfiledir* dfile);  
~hexfileviewer();

public slots:
void save_all();
void setChanged();

signals:
void changed(); 

};

#endif
