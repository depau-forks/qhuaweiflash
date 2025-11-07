//-------------- NVRAM binary image editor ----------------------------
#include "nvexplorer.h"
#include "sio.h"
#include "hexeditor.h"

//**************************************************
//* Class constructor
//**************************************************
nvexplorer::nvexplorer(uint8_t* xsrcdata, uint32_t srclen) : QMainWindow() {
 
uint32_t pos;
uint32_t i;
QString str;
QFont font;

// save the parameters of the data buffer
srcdata=xsrcdata;
plen=srclen;

// create a local copy for editing
pdata=new uint8_t[plen];
memcpy(pdata,srcdata,plen);

// window geometry settings
show();  
setAttribute(Qt::WA_DeleteOnClose);

config=new QSettings("forth32","qhuaweiflash",this);
QRect rect=config->value("/config/NvExplorerRect").toRect();
if (rect != QRect(0,0,0,0)) setGeometry(rect);
// bring the window to the foreground
setFocus();
raise();
activateWindow();

// Window title
setWindowTitle("Editing NVRAM image");


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

// Load nv header

memcpy(&nvhd, pdata, sizeof(nvhd));

// if (nvhd.magicnum != FILE_MAGIC_NUM) {
//   QMessageBox::critical(0,"Error","NVRAM image structure error - incorrect header signature");
//   delete this;
// }

// Determine the CRC type
switch (nvhd.crcflag) {
  case 0:
    crcmode=0;
    break;
    
  case 1:  
    crcmode=1;
    break;
    
  case 8:
    crcmode=2;
    break;
    
  default:
    crcmode=-1;
    break;
    
}
//----- Read the file catalog

pos=nvhd.ctrl_size; // offset to the beginning of the data (end of control structures)

// size of the file descriptor in the nvram image
uint32_t fcsize=sizeof(struct nv_file)-4;
// offset to the current file descriptor
uint32_t fsoffset;

// extract all file descriptors
for(i=0;i<nvhd.file_num;i++) {
 fsoffset=i*fcsize; 
 memcpy(&flist[i],pdata+nvhd.file_offset+fsoffset,fcsize); 
 // calculate the offset to the file data
 flist[i].offset=pos;
 pos+=flist[i].size;
}

// get the offset to the CRC field
crcoff=pos;

//----- Read the cell catalog
itemlist=new struct nv_item[nvhd.item_size];
memcpy(itemlist,pdata+nvhd.item_offset,nvhd.item_size);

// Calculate the maximum cell size
// for(i=0;i<nvhd.item_count;i++) 
//  if (maxitemlen < itemlist[i].len) maxitemlen = itemlist[i].len;
  
// Create nvram table
nvtable=new QTableWidget(nvhd.item_count,5,central);

// table header
QStringList plst;
plst << "NVID" << "Size" <<"Component" <<"Name" <<"Content";
nvtable->setHorizontalHeaderLabels(plst);

// display the list of cells in the table
QTableWidgetItem* cell;


for(i=0;i<nvhd.item_count;i++) {
  // cell id
  str.setNum(itemlist[i].id);
  cell=new QTableWidgetItem(str);
  cell->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  font=cell->font();
  font.setBold(true);
  cell->setFont(font);
  nvtable->setItem(i,0,cell);

  // cell size
  str.setNum(itemlist[i].len);
  cell=new QTableWidgetItem(str);
  cell->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  nvtable->setItem(i,1,cell);

  // component
  int fid=itemlist[i].file_id;
  str.sprintf("%1i:%s",fid,flist[fileidx(fid)].name);
  cell=new QTableWidgetItem(str);
  cell->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  nvtable->setItem(i,2,cell);

  // name
  str=find_desc(itemlist[i].id);
  cell=new QTableWidgetItem(str);
  cell->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  nvtable->setItem(i,3,cell);

  // Content  
  datacell(i);
}

// column width
for(i=0;i<4;i++) {
   nvtable->resizeColumnToContents(i);
}  
// expand the ID field for better readability
nvtable->setColumnWidth(0,nvtable->columnWidth(0)+5);
// expand the content field to the maximum
nvtable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

// Enter the table into the layout
vlm->addWidget(nvtable,3);

//-----------------------------------------------------------------------------------------------------------------------------------------------
// menu items
menu_file->addAction(QIcon::fromTheme("document-save"),"Save",this,SLOT(save_all()),QKeySequence::Save);
toolbar->addAction(QIcon::fromTheme("document-save"),"Save",this,SLOT(save_all()));
menu_file->addSeparator();
menu_file->addAction("Exit",this,SLOT(close()),QKeySequence("Esc"));

toolbar->addSeparator();

menu_edit->addAction(QIcon(":/icon_hex.png"),"Edit cell",this,SLOT(edititem()),QKeySequence("F2"));
toolbar->addAction(QIcon(":/icon_hex.png"),"Edit cell",this,SLOT(edititem()));

menu_edit->addAction(QIcon(":/icon_extract.png"),"Extract cell to file",this,SLOT(extract_item()),QKeySequence("F11"));
toolbar->addAction(QIcon(":/icon_extract.png"),"Extract cell to file",this,SLOT(extract_item()));

menu_edit->addAction(QIcon::fromTheme("object-flip-vertical"),"Load cell from file",this,SLOT(replace_item()),0);
toolbar->addAction(QIcon::fromTheme("object-flip-vertical"),"Load cell from file",this,SLOT(replace_item()));

menu_view->addAction(QIcon::fromTheme("zoom-in"),"Increase font",this,SLOT(zoomin()),QKeySequence("Ctrl++"));
toolbar->addAction(QIcon::fromTheme("zoom-in"),"Increase font",this,SLOT(zoomin()));
menu_view->addAction(QIcon::fromTheme("zoom-out"),"Decrease font",this,SLOT(zoomout()),QKeySequence("Ctrl+-"));
toolbar->addAction(QIcon::fromTheme("zoom-out"),"Decrease font",this,SLOT(zoomout()));

connect(nvtable,SIGNAL(cellActivated(int,int)),SLOT(edititem()));


}

//**********************************************************************
//*  Class DEstructor
//**********************************************************************
nvexplorer::~nvexplorer() {

int reply;

if (changed) {
  reply=QMessageBox::warning(this,"Write data","NVRAM content has been changed, save?",QMessageBox::Ok | QMessageBox::Cancel);
  if (reply == QMessageBox::Ok) {
    // saving data
    save_all();
  }
}  

// main window geometry
QRect rect=geometry();
config->setValue("/config/NvExplorerRect",rect);

delete nvtable;
delete [] itemlist;
delete [] pdata;
}
//**********************************************************************
//*  Entering the contents of a cell into a table
//**********************************************************************
void nvexplorer::datacell(int row) {
  
char dstr[10];
QString str;
uint32_t j;
uint32_t off=itemoff_idx(row);
QTableWidgetItem* cell;

uint32_t itemlen=itemlist[row].len;

for(j=0;j<itemlen;j++) {
  sprintf(dstr,"%02X ",*((uint8_t*)(pdata+off+j))&0xff);
  str.append(dstr);
}
    
cell=new QTableWidgetItem(str);
cell->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
QFont font=cell->font();
font.setFixedPitch(true);
cell->setFont(font);
nvtable->setItem(row,4,cell);
}


//**********************************************************************
//* Increase/decrease font
//**********************************************************************
void nvexplorer::zoom (int dir) {
  
QFont font;
int row,col;

for(row=0;row<nvtable->rowCount();row++) {
  for(col=0;col<nvtable->columnCount();col++) {
    font=nvtable->item(row,col)->font();
    font.setPointSize(font.pointSize()+dir);
    nvtable->item(row,col)->setFont(font);
  }
}
// column width
for(col=0;col<4;col++) {
   nvtable->resizeColumnToContents(col);
}  
// expand the ID field for better readability
nvtable->setColumnWidth(0,nvtable->columnWidth(0)+5);

}

//**********************************************************************
//* Zoom slots
//**********************************************************************
void nvexplorer::zoomin() { zoom(1); }
void nvexplorer::zoomout() { zoom(-1); }
    

//**********************************************************************
//* Cell editor
//**********************************************************************
void nvexplorer::edititem() {
 
QString title;  
  
int row=nvtable->currentRow();
uint32_t len=itemlist[row].len;
int res;

// load data into the buffer for editing
QByteArray hexcup((char*)(pdata+itemoff_idx(row)),len);

// dialog panel
QDialog* qd=new QDialog;
QVBoxLayout* vlm=new QVBoxLayout(qd);

// title
title.sprintf("Editing cell %i",itemlist[row].id);
config=new QSettings("forth32","qhuaweiflash",this);
qd->setWindowTitle(title);

// window size
QRect rect=config->value("/config/ItemEditorRect").toRect();
if (rect != QRect(0,0,0,0)) qd->setGeometry(rect);
else qd->resize(625,625);


// HEX-editor
QHexEdit* dhex=new QHexEdit(qd);

// Customize the appearance of the editor
dhex->setAddressWidth(3);
dhex->setOverwriteMode(true);
dhex->setHexCaps(true);
dhex->setHighlighting(true);

// Loading data into the editor
dhex->setData(hexcup);

dhex->setCursorPosition(0);
dhex->show();
dhex->setReadOnly(false);

vlm->addWidget(dhex);

// comments on shortcuts
QLabel* lbl=new QLabel("Enter - save changes,   Esc - cancel",qd);
vlm->addWidget(lbl);

// save and cancel buttons
QDialogButtonBox* butt=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,Qt::Horizontal,qd);
butt->button(QDialogButtonBox::Save)->setShortcut(QKeySequence(Qt::Key_Return));
butt->button(QDialogButtonBox::Cancel)->setShortcut(QKeySequence(Qt::Key_Escape));
connect(butt, SIGNAL(accepted()), qd, SLOT(accept()));
connect(butt, SIGNAL(rejected()), qd, SLOT(reject()));
vlm->addWidget(butt);

res=qd->exec();
if (res == QDialog::Accepted) {
  // changes accepted
  if (dhex->isModified()) {
   hexcup=dhex->data();
   memcpy(pdata+itemoff_idx(row),hexcup.data(),len);
   changed_item(row);
  } 
}
// window geometry
rect=qd->geometry();
config->setValue("/config/ItemEditorRect",rect);

delete qd;
}



//**********************************************************************
//* Post-processing of cell modification
//**********************************************************************
void nvexplorer::changed_item(int row) {

QString title;  

// recalculate individual CRC - this is not required for firmware image files
//   if (crcmode == 2) restore_item_crc(row);

// Enter an asterisk in the title
if (!changed) {
    // enter an asterisk in the title
    title=windowTitle();
    title.append(" *");
    setWindowTitle(title);
    changed=true;
}  
// redraw the data line in the table
datacell(row);
}

//**********************************************************************
//* Saving all changes back to the source buffer
//**********************************************************************
void nvexplorer::save_all() {

QString str;  
int pos;  

// recalculate block CRC
if (crcmode == 1) recalc_crc();

// copy the entire array out
memcpy(srcdata,pdata,plen);
  
// remove the asterisk from the title
str=windowTitle();
pos=str.indexOf('*');
if (pos != -1) {
  str.truncate(pos-1);
  setWindowTitle(str);
}  

changed=false;
}

//**********************************************************************
//* Extracting a cell to a file
//**********************************************************************
void nvexplorer::extract_item() {

QString filename;

int row=nvtable->currentRow();
filename.sprintf("nvitem-%05i.bin",itemlist[row].id);
filename=QFileDialog::getSaveFileName(this,"Saved file name",filename,"All files (*.*)");
if (filename.isEmpty()) return;
  
QFile out(filename,this);
if (!out.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(0,"Error","File creation error");
    return;
}
out.write((char*)(pdata+itemoff_idx(row)),itemlist[row].len);
out.close();
}

//**********************************************************************
//* Loading a cell from a file
//**********************************************************************
void nvexplorer::replace_item() {

QString filename;
QString str;

int row=nvtable->currentRow();
filename.sprintf("nvitem-%05i.bin",itemlist[row].id);
filename=QFileDialog::getOpenFileName(this,"File name",filename,"All files (*.*)");
if (filename.isEmpty()) return;
  
QFile in(filename,this);
if (!in.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(0,"Error","Error opening file");
    return;
}

if (in.size() != itemlist[row].len) {
    in.close();
    str.sprintf("File size (%i) does not match cell size (%i)",(uint32_t)in.size(),(uint32_t)itemlist[row].len);
    QMessageBox::critical(0,"Error",str);
    return;
}  


in.read((char*)(pdata+itemoff_idx(row)),itemlist[row].len);
in.close();
changed_item(row);

}


  