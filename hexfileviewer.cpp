#include "hexfileviewer.h"

//***********************************************************
//* HEX-viewer constructor
//***********************************************************
hexfileviewer::hexfileviewer(cpfiledir* dfile) : QMainWindow() {
  
QString title;


setAttribute(Qt::WA_DeleteOnClose);


// window geometry settings
config=new QSettings("forth32","qhuaweiflash",this);
QRect rect=config->value("/config/HexFileEditorRect").toRect();
if (rect != QRect(0,0,0,0)) setGeometry(rect);
show();  

// bring the window to the foreground
setFocus();
raise();
activateWindow();

// save input parameters for the future  
fileptr=dfile;

// copy data to a local buffer
plen=fileptr->fsize();
pdata=new uint8_t[plen];
memcpy(pdata,fileptr->fdata(),plen);

// window title
title="HEX-View - ";
title.append(fileptr->fname());
setWindowTitle(title);

// Main menu
menubar = new QMenuBar(this);
setMenuBar(menubar);

menu_file = new QMenu("File",menubar);
menubar->addAction(menu_file->menuAction());

// Status bar
statusbar = new QStatusBar(this);
setStatusBar(statusbar);

// Central widget
central=new QWidget(this);
setCentralWidget(central);

// menu items
menu_file->addAction(QIcon::fromTheme("document-save"),"Save",this,SLOT(save_all()),QKeySequence::Save);
menu_file->addSeparator();
menu_file->addAction("Exit",this,SLOT(close()),QKeySequence("Esc"));

// main layout
vlm=new QVBoxLayout(central);

// hex-editor
hed=new hexeditor((char*)pdata,plen,menubar,statusbar,central);
vlm->addWidget(hed,2);

// modification slot
connect(hed,SIGNAL(dataChanged()),this,SLOT(setChanged()));

hed->setFocus();
}

//***********************************************************
//* Viewer destructor
//***********************************************************
hexfileviewer::~hexfileviewer() {

QMessageBox::StandardButton reply;

// main window geometry
QRect rect=geometry();
config->setValue("/config/HexFileEditorRect",rect);

// data change indicator
if (datachanged) {
  reply=QMessageBox::warning(this,"Write file","The file content has been changed, save?",QMessageBox::Ok | QMessageBox::Cancel);
  if (reply == QMessageBox::Ok) {
    // saving data
    save_all();
  }
}  
delete hed;  
delete config;
delete pdata;  
}

//***********************************************************
//* Saving data to a file vector
//***********************************************************
void hexfileviewer::save_all() {

QByteArray xdata;
QString str;
int pos;

xdata=hed->dhex->data();
memcpy(pdata,xdata.data(),plen);

fileptr->replace_data((uint8_t*)pdata,plen);

// remove the asterisk from the title
str=windowTitle();
pos=str.indexOf('*');
if (pos != -1) {
  str.truncate(pos-1);
  setWindowTitle(str);
}  
// call the modification signal
emit changed();

// restore the modification handler
datachanged=false;
connect(hed,SIGNAL(dataChanged()),this,SLOT(setChanged()));

}

//***********************************************************
//* Calling an external modification slot
//***********************************************************
void hexfileviewer::setChanged() { 

QString str;

datachanged=true;
// disconnect the signal - it is needed only once
disconnect(hed,SIGNAL(dataChanged()),this,SLOT(setChanged()));
// add an asterisk to the title
str=windowTitle();
str.append(" *");
setWindowTitle(str);
}
