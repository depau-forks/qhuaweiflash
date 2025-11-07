// 
//  cpio partition editor
// 
#include <QtCore/QVariant>
#include <QtWidgets>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "MainWindow.h"
#include "cpio.h"
#include "viewer.h"
#include "hexfileviewer.h"

//*********************************************************************
//* cpio editor class constructor
//*********************************************************************
cpioedit::cpioedit (int xpnum,QMenuBar* mbar, QWidget* parent) : QWidget(parent) {

int res;
char filename[512];

menubar=mbar;  
pnum=xpnum;
// partition image
pdata=ptable->iptr(pnum);
plen=ptable->psize(pnum);

// window layout
vlm=new QVBoxLayout(this);

// toolbar
toolbar=new QToolBar("File operations",this);
vlm->addWidget(toolbar);

// editor menu
menu_edit = new QMenu("CPIO-Editor",menubar);
menubar->addAction(menu_edit->menuAction());

// Editor menu items
menu_edit->addAction(QIcon::fromTheme("go-up"),"Go up one level",this,SLOT(go_up()),QKeySequence("Backspace"));
menu_edit->addAction(QIcon::fromTheme("document-save"),"Extract file",this,SLOT(extract_file()),QKeySequence("F11"));
menu_edit->addAction(QIcon::fromTheme("object-flip-vertical"),"Replace file",this,SLOT(replace_file()),0);
menu_edit->addAction(QIcon::fromTheme("edit-delete"),"Delete file",this,SLOT(delete_file()),QKeySequence("Del"));
menu_edit->addAction(QIcon(":/icon_hex.png"),"HEX-viewer/editor",this,SLOT(hexedit_file()),QKeySequence("F2"));
menu_edit->addAction(QIcon(":/icon_view.png"),"Text view",this,SLOT(view_file()),QKeySequence("F3"));
menu_edit->addAction(QIcon(":/icon_edit.png"),"Text editor",this,SLOT(edit_file()),QKeySequence("F4"));
menu_edit->addAction(QIcon::fromTheme("list-add"),"Add new file",this,SLOT(add_file()),QKeySequence("+"));
menu_edit->addAction(QIcon::fromTheme("folder-new"),"Create directory",this,SLOT(add_dir()),QKeySequence("F7"));

menu_edit->addSeparator();
menu_edit->addAction(QIcon::fromTheme("file-save"),"Save changes",this,SLOT(saveall()),QKeySequence("Ctrl+W"));

// Toolbar items
toolbar->addAction(QIcon::fromTheme("go-up"),"Go up one level",this,SLOT(go_up()));
toolbar->addAction(QIcon::fromTheme("document-save"),"Extract file",this,SLOT(extract_file()));
toolbar->addAction(QIcon::fromTheme("object-flip-vertical"),"Replace file",this,SLOT(replace_file()));
toolbar->addAction(QIcon::fromTheme("edit-delete"),"Delete file",this,SLOT(delete_file()));
toolbar->addAction(QIcon(":/icon_hex.png"),"HEX-viewer/editor",this,SLOT(hexedit_file()));
toolbar->addAction(QIcon(":/icon_view.png"),"Text view",this,SLOT(view_file()));
toolbar->addAction(QIcon(":/icon_edit.png"),"Text editor",this,SLOT(edit_file()));
toolbar->addSeparator();
toolbar->addAction(QIcon::fromTheme("list-add"),"Add new file",this,SLOT(add_file()));
toolbar->addAction(QIcon::fromTheme("folder-new"),"Create directory",this,SLOT(add_dir()));
toolbar->setEnabled(false);
// close access to the menu
menu_edit->setEnabled(false);

// load all cpio into lists
uint8_t* iptr=pdata;  // pointer to the current position in the partition image
rootdir=new QList<cpfiledir*>;
// cpio stream parsing cycle
while(iptr < (pdata+plen)) {
  // Search for the header signature of the next file
  while(1) {
   if (iptr >= (pdata+plen)) {
     QMessageBox::critical(0,"CPIO Error","TRAILER!!! stream delimiter not found");
     goto ldone;
   }  
   if (is_cpio(iptr)) break; // found signature
   iptr++; // look for it further
  }  
 extract_filename(iptr,filename);
 if (strncmp(filename,"TRAILER!!!",10) == 0) break;
 res=cpio_load_file(iptr,rootdir,plen,filename);
 if (res == 0) break;
 iptr+=res;
}
ldone:
// display the root directory
cpio_show_dir(rootdir,0);

}

//*********************************************************************
//* cpio class destructor
//*********************************************************************
cpioedit::~cpioedit () {

QMessageBox::StandardButton reply;

// Check if anything has changed inside  
if (is_modified) {
  reply=QMessageBox::warning(this,"Write partition","The content of the partition has been changed, save?",QMessageBox::Ok | QMessageBox::Cancel);
  if (reply == QMessageBox::Ok) repack_cpio();
}  
// delete root directory elements
qDeleteAll(*rootdir);
// clear the root directory
rootdir->clear();  
// delete the root directory
delete rootdir;

// destroy the menu
delete menu_edit;

}


//*********************************************************************
//* Opening the toolbar and menu
//*********************************************************************
void cpioedit::menuenabler() {
  
toolbar->setEnabled(true);
menu_edit->setEnabled(true);
// disconnect this slot - it has already worked and is no longer needed
disconnect(cpiotable,0,this,SLOT(menuenabler()));
}

//*********************************************************************
//* Saving changes
//*********************************************************************
void cpioedit::saveall() {
  
repack_cpio();
is_modified=false;
}

//*************************************************************
//*  Formation of the list of files
//*
//* focusmode - allows setting focus on the view window
//*************************************************************
void cpioedit::cpio_show_dir(QList<cpfiledir*>* dir, int focusmode) {

QTableWidgetItem* item;
QString str;
QStringList(plst);
QStringList(hlist);

int i,j;
time_t ctime;
char tstr[100];
uint32_t fm;
char modestr[10];
int showsize;

cpiotable=new QTableWidget(0,7,this);

plst <<"idx" << "Name" << "size" << "Date" << "Mode" << "GID" << "UID"; 
cpiotable->setHorizontalHeaderLabels(plst);

currentdir=dir;

cpiotable->setRowCount(dir->count()); //cpiotable->rowCount()+1);
for (i=0;i<dir->count();i++) {
  hlist <<""; 
  // file index in the vector
  str.sprintf("%i",i);
  item=new QTableWidgetItem(str);
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  item->setForeground(QBrush(Qt::black));
  cpiotable->setItem(i,0,item);
  
  // file name
  str=dir->at(i)->cfname();
  item=new QTableWidgetItem(str);
  // Select file icon
  showsize=0;
  if (i == 0) item->setIcon(QIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowBack))); 
  else if (dir->at(i)->subdir != 0) item->setIcon(QIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon))); 
  else if (((dir->at(i)->fmode())&C_ISLNK) == C_ISLNK) {
    // symlink
    item->setIcon(QIcon(QApplication::style()->standardIcon(QStyle::SP_FileLinkIcon)));
    // add a link to the file name to the symlink name
    str.append(" -> ");
    str.append(dir->at(i)->fdata()); 
    item->setText(str);
  }  
  else  {
    // executable files
    if ((((dir->at(i)->fmode())&C_IXUSR) != 0)) item->setIcon(QIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon)));
    // non-executable files
    else item->setIcon(QIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon)));
    // allow to show size
    showsize=1;
  }  
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  cpiotable->setItem(i,1,item);
  if (i == 0) continue;

  // file size
  if (showsize) {
   str.sprintf("%i",dir->at(i)->fsize());
   item=new QTableWidgetItem(str);
   item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
   item->setForeground(QBrush(Qt::blue));
   cpiotable->setItem(i,2,item);
  } 
  
  // date-time
  ctime=dir->at(i)->ftime();
  strftime(tstr,100,"%d-%b-%y  %H:%M",localtime(&ctime));
  str=tstr;
  item=new QTableWidgetItem(str);
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  item->setForeground(QBrush(Qt::black));
  cpiotable->setItem(i,3,item);
  
  // access attributes
  fm=dir->at(i)->fmode();
  strcpy(modestr,"rwxrwxrwx");
  for (j=0;j<9;j++) {
    if (((fm>>j)&1) == 0) modestr[8-j]='-';
  }  
  str=modestr;
  item=new QTableWidgetItem(str);
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  item->setForeground(QBrush(Qt::red));
  cpiotable->setItem(i,4,item);
  
  // gid
  str.sprintf("%i",dir->at(i)->fgid());
  item=new QTableWidgetItem(str);
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  item->setForeground(QBrush(Qt::black));
  cpiotable->setItem(i,5,item);
  
  // uid
  str.sprintf("%i",dir->at(i)->fuid());
  item=new QTableWidgetItem(str);
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  item->setForeground(QBrush(Qt::black));
  cpiotable->setItem(i,6,item);

} 
  //------------------------------------
//   cpiotable->setEditTriggers(QAbstractItemView::NoEditTriggers);

  
  // hide file indexes
  cpiotable->setColumnHidden(0,true);
  
  // hide vertical headers
  cpiotable->setVerticalHeaderLabels(hlist);

  
  cpiotable->resizeColumnsToContents();
  cpiotable->setShowGrid(false);
  cpiotable->setColumnWidth(1, 250);
  cpiotable->setColumnWidth(2, 100);

  cpiotable->sortByColumn(1,Qt::AscendingOrder);
  
  // File selection signal (enter or double click)
  connect(cpiotable,SIGNAL(cellActivated(int,int)),SLOT(cpio_process_file(int,int)));
//   connect(cpiotable,SIGNAL(cellDoubleClicked(int,int)),SLOT(cpio_process_file(int,int)));
//   connect(cpiotable,SIGNAL(cellPressed(int,int)),SLOT(cpio_process_file(int,int)));
  
  // Enter the table into the screen form
  vlm->addWidget(cpiotable);
  cpiotable->show();
  cpiotable->setCurrentCell(0,0);
  if (focusmode) {
    // The table gets the focus - the menu can be opened
    cpiotable->setFocus();
    menuenabler();
  }
  // The table does not get focus - set a trap
  else {
    connect(cpiotable,SIGNAL(cellActivated(int,int)),this,SLOT(menuenabler()));
    connect(cpiotable,SIGNAL(cellClicked(int,int)),this,SLOT(menuenabler()));
  }  
    
}

//*********************************************************************
//* Destruction of the file table
//*********************************************************************
void cpioedit::cpio_hide_dir() {

disconnect(cpiotable,0,this,0);  

vlm->removeWidget(cpiotable);
  
delete cpiotable;
cpiotable=0;
}

//*********************************************************************
//* Getting the index of the current file in the directory vector
//*********************************************************************
int cpioedit::current_file_index() {

QTableWidgetItem* item;
QString qfn;
int idx;
int row=cpiotable->currentRow();
item=cpiotable->item(row,0);
qfn=item->text();
idx=qfn.toUInt();
return idx;
}

//*********************************************************************
//* Getting a link to the descriptor of the current file
//*********************************************************************
cpfiledir* cpioedit::selected_file() {

return currentdir->at(current_file_index());
}

//*********************************************************************
//* Deleting a file
//*********************************************************************
void cpioedit::delete_file() {
  
int idx;
int row=cpiotable->currentRow();

idx=current_file_index(); // file position in the vector
delete selected_file();   // delete file descriptor
currentdir->removeAt(idx); // remove the file from the list
// redraw the table
cpio_hide_dir();
cpio_show_dir(currentdir,true);
cpiotable->setCurrentCell(row,0);
}


//*********************************************************************
//* file extraction
//*********************************************************************
void cpioedit::extract_file() {

FILE* out;  
cpfiledir* fd;

fd=selected_file();

if (((fd->fmode()) & C_ISREG) == 0) {
  // irregular file - it cannot be extracted
  QMessageBox::critical(0,"Error","Irregular files cannot be extracted");  
  return;
}

QString fn=fd->cfname();

fn=QFileDialog::getSaveFileName(this,"Saving file",fn,"All files (*.*)");
if (fn.isEmpty()) return;
out=fopen(fn.toLocal8Bit().data(),"w");
fwrite(fd->fdata(),1,fd->fsize(),out);
fclose(out);
}

//*********************************************************************
//* file replacement
//*********************************************************************
void cpioedit::replace_file() {

cpfiledir* fd;
QString fn;
uint32_t fsize;

fd=selected_file();

if (((fd->fmode()) & C_ISREG) == 0) {
  // irregular file - it cannot be extracted
  QMessageBox::critical(0,"Error","Irregular files cannot be replaced");  
  return;
}

fn=QFileDialog::getOpenFileName(this,"Replacing a file",fn,"All files (*.*)");
if (fn.isEmpty()) return;

QFile in(fn,this);
if (!in.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(0,"Error","File read error");
    return;
}
fsize=in.size();
uint8_t* fbuf=new uint8_t[fsize]; // file buffer
in.read((char*)fbuf,fsize);
in.close();
fd->replace_data(fbuf,fsize);
delete [] fbuf;
}

//*********************************************************************
//* Calling the file editor
//*********************************************************************
void cpioedit::fileeditor(bool readonly) {

cpfiledir* fd;

fd=selected_file();

if (((fd->fmode()) & C_ISREG) == 0) {
  // irregular file - it cannot be extracted
  QMessageBox::critical(0,"Error","Irregular files cannot be viewed/edited");  
  return;
}

view=new viewer(0,0,readonly,fd->fname(),fd);  
// modification signal
connect(view,SIGNAL(changed()),this,SLOT(setModified()));

}  

//*********************************************************************
//* text editor
//*********************************************************************
void cpioedit::view_file() { fileeditor(true); }
void cpioedit::edit_file() { fileeditor(false); }

//*********************************************************************
//* Calling the hex editor
//*********************************************************************
void cpioedit::hexedit_file() {

cpfiledir* fd;

fd=selected_file();

if (((fd->fmode()) & C_ISREG) == 0) {
  // irregular file - it cannot be extracted
  QMessageBox::critical(0,"Error","Irregular files cannot be viewed/edited");  
  return;
}

hview=new hexfileviewer(fd);  

// modification signal
connect(hview,SIGNAL(changed()),this,SLOT(setModified()));
}  

//*********************************************************************
//* Go up one level
//*********************************************************************
void cpioedit::go_up() {
  
emit cpio_process_file(0,0);
}


//*********************************************************************
//* Receiver of the file/directory selection signal
//*********************************************************************
void cpioedit::cpio_process_file(int row, int col) {

QList<cpfiledir*>* subdir;
if (row<0) return;

if (row != 0) subdir=selected_file()->subdir; // one of the subdirectories
else subdir=currentdir->at(0)->subdir; // upper level directory

if (subdir == 0) return; // selected file is not a directory
if (cpiotable != 0) { // not the root directory
  disconnect(cpiotable,0,this,0); // disconnect all slots
  cpio_hide_dir();
  cpio_show_dir(subdir,1);
}  
}

//*********************************************************************
//* Repacking the cpio partition back
//*********************************************************************
void cpioedit::repack_cpio() {
  
uint8_t* ndata=new uint8_t[fullsize(rootdir)+4096];
uint32_t nlen=0;
int i;

for(i=0;i<rootdir->count(); i++) {
  nlen+=rootdir->at(i)->store_cpio(ndata+nlen);
}

// cpio file tail
bzero(ndata+nlen,128);
strcpy((char*)(ndata+nlen),"07070100000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000B00000000TRAILER!!!");
nlen+=128;

ptable->replace(pnum,ndata,nlen);
pdata=ndata;
plen=nlen;
}

//*********************************************************************
//* Adding a new file
//*********************************************************************
void cpioedit::add_file() {

cpfiledir* fd;
QString fn;
uint32_t fsize;
uint8_t* fbuf=0;
uint8_t filename[100];
char str[10];

// cpio header emulation
cpio_header_t hdr;    
// fill in the header constants
memset(&hdr,'0',sizeof(hdr));
memcpy(hdr.c_magic,"070701",6);
memcpy(hdr.c_mode,"000081B4",8);

fn=QFileDialog::getOpenFileName(this,"Adding a new file",fn,"All files (*.*)");
if (fn.isEmpty()) return;

QFile in(fn,this);
if (!in.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(0,"Error","File read error");
    return;
}
fsize=in.size();
if (fsize != 0) {
  fbuf=new uint8_t[fsize]; // file buffer
  // read the entire file into the buffer
  in.read((char*)fbuf,fsize);
}

// Get file information
QFileInfo fi=QFileInfo(in);
// date-time
sprintf(str,"%08x",fi.created().toSecsSinceEpoch()&0xffffffff);
memcpy(hdr.c_mtime,str,8);
// gid
sprintf(str,"%08x",fi.groupId());
memcpy(hdr.c_gid,str,8);
// uid
sprintf(str,"%08x",fi.ownerId());
memcpy(hdr.c_uid,str,8);
// attributes
uint32_t attr=fi.permissions()&0xfff;
attr=(attr&7) | ((attr&0xf0)>>1) | ((attr&0xf00)>>2); // convert from QT format to normal unix
attr|=0x8000;  // raise the regular file flag  //81b4  1000 000 110 110 100   6644  110 0110 0100 0100
//printf("\n attr = %08x\n",fi.permissions());
sprintf(str,"%08x",attr);
memcpy(hdr.c_mode,str,8);
// file name
fn=fi.fileName();
sprintf(str,"%08x",fn.size()+1); // file name length
memcpy(hdr.c_namesize,str,8);
memcpy(filename,fn.toLocal8Bit().data(),fn.size()+1); // file name
// file size
sprintf(str,"%08x",fsize); 
memcpy(hdr.c_filesize,str,8);

// file is no longer needed
in.close();

// create a new file record
fd=new cpfiledir(&hdr, filename, fbuf);
if (fbuf != 0) delete [] fbuf;

// add the file to the current directory
currentdir->append(fd);
cpio_hide_dir();
cpio_show_dir(currentdir,true);

}

//*********************************************************************
//* Creating a directory
//*********************************************************************
void cpioedit::add_dir() {

cpfiledir* fd;
char dirname[100];
char str[10];

int res;

// cpio header emulation
cpio_header_t hdr;    
// fill in the header constants
memset(&hdr,'0',sizeof(hdr));
memcpy(hdr.c_magic,"070701",6);
memcpy(hdr.c_mode,"000041ED",8);

QInputDialog* pd=new QInputDialog(this);  
pd->setLabelText("Directory name:");
res=pd->exec();

if (res == QDialog::Accepted) {
 // answer received - create a directory   
 strcpy(dirname,pd->textValue().toLocal8Bit().data());  // directory name 
 // date-time
 sprintf(str,"%08x",time(0));
 memcpy(hdr.c_mtime,str,8);
 // directory name length
 sprintf(str,"%08x",strlen(dirname)+1);
 memcpy(hdr.c_namesize,str,8);

 // create a new file record
 fd=new cpfiledir(&hdr, dirname, 0);

  // subdirectory vector
 fd->subdir=new QList<cpfiledir*>;
 // pointer to the upper-level directory (i.e. this one)
 cpfiledir* upfd=new cpfiledir(&hdr," ",0);
 upfd->subdir=currentdir;
 // file name for it is ".."
 upfd->setfname("..");
 upfd->updirflag=true;  // sign of a link to the upper level directory
 // add this entry first to the subdirectory vector
 fd->subdir->append(upfd);

// add the file to the current directory
 currentdir->append(fd);
 cpio_hide_dir();
 cpio_show_dir(currentdir,true);

}
delete pd;

}
