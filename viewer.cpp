// view and edit arbitrary files 

#include "viewer.h"

//***********************************************************
//* Viewer constructor
//***********************************************************
viewer::viewer(uint8_t* srcdata, uint32_t* srclen, uint8_t rmode, char* fname, cpfiledir* dfile) : QMainWindow() {
  
QString title;
QFont font;
uint32_t plen;

// window geometry settings
show();  
setAttribute(Qt::WA_DeleteOnClose);

config=new QSettings("forth32","qhuaweiflash",this);
QRect rect=config->value("/config/EditorRect").toRect();
if (rect != QRect(0,0,0,0)) setGeometry(rect);
// bring the window to the foreground
setFocus();
raise();
activateWindow();

// save input parameters for the future  
fileptr=dfile;
readonly=rmode;
sdata=srcdata;
slen=srclen;

// determine the buffer size and create it
if (fileptr != 0)  plen=fileptr->fsize();
else plen=*slen;
pdata=new uint8_t[plen+1];

// copy data to a local buffer
if (fileptr != 0) memcpy(pdata,fileptr->fdata(),plen);
else memcpy(pdata,srcdata,plen);

// line delimiter
pdata[plen]=0; 

// window title
if (readonly) title="View - ";
else title="Editing - ";
title.append(fname);
setWindowTitle(title);

// Main menu
menubar = new QMenuBar(this);
setMenuBar(menubar);

menu_file = new QMenu("File",menubar);
menubar->addAction(menu_file->menuAction());

menu_edit = new QMenu("Edit",menubar);
menubar->addAction(menu_edit->menuAction());

menu_view = new QMenu("View",menubar);
menubar->addAction(menu_view->menuAction());


// toolbar
toolbar=new QToolBar(this);
addToolBar(toolbar);

// Central widget
central=new QWidget(this);
setCentralWidget(central);

// main layout
vlm=new QVBoxLayout(central);

// text editor
ted=new QTextEdit(central);
ted->setReadOnly(readonly);
vlm->addWidget(ted,2);

// editor font
font=qvariant_cast<QFont>(config->value("/config/EditorFont"));
ted->setFont(font);

// filling the text editor
textdata=(char*)pdata;
ted->append(textdata);

// menu items
menu_file->addAction(QIcon::fromTheme("document-save"),"Save",this,SLOT(save_all()),QKeySequence::Save);
toolbar->addAction(QIcon::fromTheme("document-save"),"Save",this,SLOT(save_all()));
menu_file->addSeparator();
menu_file->addAction("Exit",this,SLOT(close()),QKeySequence("Esc"));

toolbar->addSeparator();

if (!readonly) {
  menu_edit->addAction(QIcon::fromTheme("edit-undo"),"Cancel",ted,SLOT(undo()),QKeySequence::Undo);
  toolbar->addAction(QIcon::fromTheme("edit-undo"),"Cancel",ted,SLOT(undo()));
  menu_edit->addAction(QIcon::fromTheme("edit-redo"),"Redo",ted,SLOT(redo()),QKeySequence::Redo);
  toolbar->addAction(QIcon::fromTheme("edit-redo"),"Redo",ted,SLOT(redo()));
  menu_edit->addSeparator();
  menu_edit->addAction(QIcon::fromTheme("edit-cut"),"Cut",ted,SLOT(cut()),QKeySequence::Cut);
  toolbar->addAction(QIcon::fromTheme("edit-cut"),"Cut",ted,SLOT(cut()));
}
menu_edit->addAction(QIcon::fromTheme("edit-copy"),"Copy",ted,SLOT(copy()),QKeySequence::Copy);
toolbar->addAction(QIcon::fromTheme("edit-copy"),"Copy",ted,SLOT(copy()));

if (!readonly) {
  menu_edit->addAction(QIcon::fromTheme("edit-paste"),"Paste",ted,SLOT(paste()),QKeySequence::Paste);
  toolbar->addAction(QIcon::fromTheme("edit-paste"),"Paste",ted,SLOT(paste()));
  toolbar->addSeparator();
}
menu_edit->addAction(QIcon::fromTheme("edit-find"),"Find...",this,SLOT(find()),QKeySequence::Find);
toolbar->addAction(QIcon::fromTheme("edit-find"),"Find...",this,SLOT(find()));
menu_edit->addAction(QIcon::fromTheme("edit-find"),"Find next",this,SLOT(findnext()),QKeySequence::FindNext);


menu_view->addAction(QIcon::fromTheme("zoom-in"),"Increase font",ted,SLOT(zoomIn()),QKeySequence("Ctrl++"));
toolbar->addAction(QIcon::fromTheme("zoom-in"),"Increase font",ted,SLOT(zoomIn()));
menu_view->addAction(QIcon::fromTheme("zoom-out"),"Decrease font",ted,SLOT(zoomOut()),QKeySequence("Ctrl+-"));
toolbar->addAction(QIcon::fromTheme("zoom-out"),"Decrease font",ted,SLOT(zoomOut()));
menu_view->addAction(QIcon::fromTheme("preferences-desktop-font"),"Font...",this,SLOT(fontselector()));

// modification slot
connect(ted,SIGNAL(textChanged()),this,SLOT(setChanged()));

ted->setFocus();
ted->moveCursor(QTextCursor::Start,QTextCursor::MoveAnchor);
}

//***********************************************************
//* Viewer destructor
//***********************************************************
viewer::~viewer() {

QMessageBox::StandardButton reply;
QFont font;

// save the font size
font=ted->font();
config->setValue("/config/EditorFont",font);

// main window geometry
QRect rect=geometry();
config->setValue("/config/EditorRect",rect);

// data change indicator
if (datachanged) {
  reply=QMessageBox::warning(this,"Write file","The file content has been changed, save?",QMessageBox::Ok | QMessageBox::Cancel);
  if (reply == QMessageBox::Ok) {
    // saving data
    save_all();
  }
}  
  
delete config;
delete [] pdata;  
}

//***********************************************************
//* Saving data to a file vector
//***********************************************************
void viewer::save_all() {

QByteArray xdata;
QString str;
int pos;

textdata=ted->toPlainText();
xdata=textdata.toLocal8Bit();
if (fileptr != 0) fileptr->replace_data((uint8_t*)xdata.data(),xdata.size());
else {
  delete [] sdata;
  sdata=new uint8_t[xdata.size()];
  memcpy(sdata,(uint8_t*)xdata.data(),xdata.size());
  *slen=xdata.size();
}  
// call the modification signal
emit changed();

// remove the asterisk from the title
str=windowTitle();
pos=str.indexOf('*');
if (pos != -1) {
  str.truncate(pos-1);
  setWindowTitle(str);
}  
// restore the modification handler
datachanged=false;
connect(ted,SIGNAL(textChanged()),this,SLOT(setChanged()));

}

//***********************************************************
//* Text search
//***********************************************************
void viewer::find() {

int res;  
  
QInputDialog* pd=new QInputDialog(this);  
pd->setLabelText("Search in file:");
res=pd->exec();
if (res == QDialog::Accepted) {
 findtext=pd->textValue();
 findnext();
}
delete pd;
}

//***********************************************************
//* Continue text search
//***********************************************************
void viewer::findnext() {

int res;

if (findtext.size() == 0) return;
  res=ted->find(findtext);
  if (!res) {
    QMessageBox::information(0,"Information","Text not found");
  } 
}  

//***********************************************************
//* Font selection
//***********************************************************
void viewer::fontselector() {

int res;

QFont font=ted->font();
QFontDialog* fss=new QFontDialog(this);
fss->setCurrentFont(font);
res=fss->exec();
if (res == QDialog::Accepted) {
  font=fss->selectedFont();
  ted->setFont(font);
}
delete fss;
}


//***********************************************************
//* Calling an external modification slot
//***********************************************************
void viewer::setChanged() { 

QString str;

datachanged=true;
// disconnect the signal - it is needed only once
disconnect(ted,SIGNAL(textChanged()),this,SLOT(setChanged()));
// add an asterisk to the title
str=windowTitle();
str.append(" *");
setWindowTitle(str);
}
