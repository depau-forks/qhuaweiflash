#include <QtWidgets>
#include <MainWindow.h>

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "sio.h"
#include "usbloader.h"
#include "fwsave.h"
#include "signver.h"
#include "parts.h"
#include "cpio.h"
#include "kerneledit.h"
#include "nvdedit.h"

void flasher();

// reference to port selector
QComboBox* pselector;

// Partition table
ptable_list* ptable;
int npart=0;

QString fwfilename;
MainWindow* mw;

//*************************************************
//  Search for ttyUSB ports and collect their names in a table
//*************************************************
void MainWindow::find_ports() {

QDir fdir("/dev");

PortSelector->clear();
PortSelector->addItems(fdir.entryList((QStringList)"ttyUSB*",QDir::System,QDir::Name));
PortSelector->setCurrentIndex(0);
}

//*****************************************
//*  Open firmware file
//*****************************************
void MainWindow::OpenFwFile(QString filename) {
  
FILE* in;
int idx;

QSettings rc("forth32","qhuaweiflash",this);
QStringList recent=rc.value("/recent/rfiles").toStringList();

in=fopen(filename.toLocal8Bit(),"r");
if (in == 0) {
  QMessageBox::critical(0,"Error","Error opening file");
  return;
}  

// add file to recent-list
idx=recent.indexOf(filename); // search for duplicates
if (idx == -1) { 
  // no duplicates found
  recent.prepend(filename);
  if (recent.count()>6) recent.removeLast();
}
else {
  // such file already exists - move it to the beginning of the list
  recent.move(idx,0);
}  
rc.setValue("/recent/rfiles",recent);


// Search for partitions and form partition table
ptable->findparts(in); 
regenerate_partlist();
partlist->setCurrentRow(0);
SelectPart(); 
// search for digital signature
if (signlen == -1) {
  // search for digital signature
  search_sign();
  printf("\n signlen = %i",signlen);
  if (signlen != -1) dload_id|=8; // insert signature presence flag
}

EnableMenu();
if (fwfilename.isEmpty()) {
  // default file name
  fwfilename=filename;
  // file name in window title
  settitle();
  QString title=windowTitle();
  title.append(" - ");
  title.append(filename);
  setWindowTitle(title);
}
// set correct firmware type in type selector
dload_id_selector->setCurrentIndex(dload_id&7);
// compression flag
if(ptable->zsize(0)) zflag_selector->setChecked(true);

}  
  
//*****************************************
//*  Append partitions from firmware file
//*****************************************
void MainWindow::AppendFwFile() {
  
QString fwname;

QFileDialog* qf=new QFileDialog(this);
fwname=qf->getOpenFileName(0,"Select firmware file",".","firmware (*.fw *.exe *.bin *.BIN);;All files (*.*)");
delete qf;
if (fwname.isEmpty()) return;
OpenFwFile(fwname);

}

//********************************************
//*  Generate on-screen partition list
//********************************************
void MainWindow::regenerate_partlist() {

int i;
QString str;
partlist->blockSignals(true);
partlist->clear();
for (i=0;i<ptable->index();i++) {
  str.sprintf("%06x - %s",ptable->code(i),ptable->name(i));
  partlist->addItem(str);
}  
hrow=-1;
partlist->blockSignals(false);
}  

//*****************************************
//*  Select new firmware file
//*****************************************
void MainWindow::SelectFwFile() {

ask_save();
  
menu_part->setEnabled(0);
Menu_Oper_flash->setEnabled(0);
fileappend->setEnabled(0);
filesave->setEnabled(0);
ptable->clear();
fwfilename.clear();
AppendFwFile();
EnableMenu();
}

//*****************************************
//* Select last opened file
//*****************************************
void MainWindow::open_recent_file() {

QString fname;
QAction *action = qobject_cast<QAction *>(sender());
if (action == 0) return;

ask_save();

menu_part->setEnabled(0);
Menu_Oper_flash->setEnabled(0);
fileappend->setEnabled(0);
filesave->setEnabled(0);
ptable->clear();
fwfilename.clear();
fname=action->text();
if (fname.isEmpty()) return;
OpenFwFile(fname);

EnableMenu();

}

  
//*****************************************
//*  Enable menu items
//*****************************************
void MainWindow::EnableMenu(){ 
  
if (ptable->index() != 0) { 
  menu_part->setEnabled(1);
  Menu_Oper_flash->setEnabled(1);
  fileappend->setEnabled(1);
  filesave->setEnabled(1);
}
if ((dload_id&8) != 0) Menu_Oper_signinfo->setEnabled(1);
}

//******************************************************************
//*  Write complete firmware file to disk with file name specification
//******************************************************************
void MainWindow::save_as() {

fw_saver(true,zflag_selector->isChecked());  
// new file name in title
settitle();
QString title=windowTitle();
title.append(" - ");
title.append(fwfilename);
setWindowTitle(title);

modified=false;
}

//*****************************************
//*  Overwrite firmware file
//*****************************************
void MainWindow::SaveFwFile() {

  
fw_saver(false,zflag_selector->isChecked());  
// remove asterisk from title
QString str=windowTitle();
int pos=str.indexOf('*');
if (pos != -1) {
  str.truncate(pos-1);
  setWindowTitle(str);
}  
modified=false;
}
  

//*****************************************
//* Prompt to save modified file
//*****************************************
void MainWindow::ask_save() {

if (modified) {
 // create save prompt panel
 QMessageBox msgBox;  
 msgBox.setText("Firmware image has been modified");
 msgBox.setInformativeText("Save changes?");
 msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard);
 msgBox.setDefaultButton(QMessageBox::Save);
 int reply = msgBox.exec();  
 if (reply == QMessageBox::Save) fw_saver(false,zflag_selector->isChecked());  
}
modified=false;
}
  

//*****************************************
//* Copy headers
//*****************************************
void MainWindow::HeadCopy() {

head_copy();
SelectPart();
}


//*********************************************
//* Remove all partition viewing elements
//*********************************************
void MainWindow::removeEditor() {

if (hexedit != 0) {
  EditorLayout->removeWidget(hexedit);
  delete hexedit;
  hexedit=0;
}  

if (kedit != 0) {
  EditorLayout->removeWidget(kedit);
  delete kedit;
  kedit=0;
}  

if (nvedit != 0) {
  EditorLayout->removeWidget(nvedit);
  delete nvedit;
  nvedit=0;
}  

if (cpio != 0) {
  EditorLayout->removeWidget(cpio);
  delete cpio;
  cpio=0;
}  

if (ptedit != 0) {
  EditorLayout->removeWidget(ptedit);
  delete ptedit;
  ptedit=0;
}  

if (oemedit != 0) {
  EditorLayout->removeWidget(oemedit);
  delete oemedit;
  oemedit=0;
}  

if (label != 0) {
  EditorLayout->removeWidget(label);
  delete label;
  label=0;
}  

if (spacer != 0) {
  EditorLayout->removeItem(spacer);
  delete spacer;
  spacer=0;
}  
}

//*****************************************
//*  Select partition from list
//*****************************************
void MainWindow::SelectPart() {

QString txt;  
QStringList(plst);

int idx=partlist->currentRow();
if (idx == -1) return; // empty list
// Check and save modified data if necessary
if ((hrow != -1)&&(hrow != idx)) {
  HeaderChanged(); // save header
  DataChanged();  // save data block
}  

if ((hrow == idx) && (structure_mode_save == structure_mode->isChecked()))  return; // false signal, same list element selected

modebuttons->hide();

structure_mode_save=structure_mode->isChecked();
hrow=idx; // save for future header write


// Output header values
txt.sprintf("%-8.8s",ptable->platform(idx));
Platform_input->setText(txt);

txt.sprintf("%-16.16s",ptable->date(idx));
Date_input->setText(txt);

txt.sprintf("%-16.16s",ptable->time(idx));
Time_input->setText(txt);

txt.sprintf("%-32.32s",ptable->version(idx));
Version_input->setText(txt);

txt.sprintf("%04x",ptable->code(idx)>>16);
pcode->setText(txt);

// remove previous editor
removeEditor();

modebuttons->show();

// Structured view modes
if (structure_mode->isChecked()) {
  
   // ptable partitions (flash partition table)
   //###########################################
   if ((ptable->ptype(idx) == part_ptable) && (is_ptable(ptable->iptr(idx)))) {
    partmode=part_ptable; 
    // create partition table editor
    ptedit=new QTableWidget(0,9 ,centralwidget);
//     ptedit->setGeometry(QRect(230, 100, 600, 470));
    plst << "Name" << "start" <<"len" <<"loadsize" <<"loadaddr" << "entry" << "flags" << "type" << "count";
    ptedit->setHorizontalHeaderLabels(plst);
    parts_fill(ptedit,ptable->iptr(idx));
    EditorLayout->addWidget(ptedit);
    ptedit->show();
    return;
   }
   
   // oeminfo partitions
   //###########################################

   if (ptable->ptype(idx) == part_oem) {
    label=new QLabel("WEBUI or DASHBOARD Version");
    label->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
    EditorLayout->addWidget(label);
    label->setTextFormat(Qt::RichText);

    QFont font;
    font.setPointSize(14);
    font.setBold(true);
    font.setWeight(75);
    label->setFont(font);
    label->show();
    
    oemedit=new QLineEdit;
    oemedit->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    EditorLayout->addWidget(oemedit);
    oemedit->setText((char*)ptable->iptr(idx));
    oemedit->show();
    
    spacer=new QSpacerItem(10,500,QSizePolicy::Minimum,QSizePolicy::Maximum);
    EditorLayout->addSpacerItem(spacer);
    
    return;
   } 
 
   // filesystem partitions
   //###########################################
   if (is_cpio(ptable->iptr(idx))) {
     cpio=new cpioedit(idx,menubar,centralwidget);
     EditorLayout->addWidget(cpio);
     return;
   }

   // kernel editor
   //###########################################
   if (memcmp(ptable->iptr(idx)+128,"ANDROID!",8) == 0) {
     kedit=new kerneledit(idx,centralwidget);
     EditorLayout->addWidget(kedit);
     return;
   }  

   // nvdload partition editor
   //###########################################
   if ((ptable->ptype(idx) == part_nvram) && (*((uint32_t*)(ptable->iptr(idx))) == NV_FILE_MAGIC)) {
     nvedit=new nvdedit(idx,centralwidget);
     EditorLayout->addWidget(nvedit);
     return;
   }  

}   
// unformatted type 
// create hex-editor window
 partmode=part_bin;
 hexedit=new hexeditor((char*)ptable->iptr(idx),ptable->psize(idx),menubar,statusbar,centralwidget);
 hexedit->setObjectName(QStringLiteral("HexEditor"));

 // populate hex-editor window data
 EditorLayout->addWidget(hexedit);
//  EditorLayout->show();
 hexedit->show();
}


//*****************************************
//*  Save partition to disk
//*****************************************
void MainWindow::Menu_Part_Store() {
  
int np=partlist->currentRow();
QString filename;
QString str;
FILE* out;
uint8_t hdr[92];


// save partition image
filename.sprintf("%02i-%08x-%s.fw",np,ptable->code(np),ptable->name(np));
filename=QFileDialog::getSaveFileName(this,"File name",filename,"firmware (*.fw);;All files (*.*)");
if (filename.isEmpty()) return;
out=fopen(filename.toLocal8Bit(),"w");
if (out == 0) {
  QMessageBox::critical(0,"Error","Error creating file");
  return;
}

// write header - upgrade state
bzero(hdr,sizeof(hdr));
hdr[0]=0x0d;
fwrite(hdr,1,sizeof(hdr),out);

ptable->save_part(np,out,0);
fclose(out);
}

//*****************************************
//*  Extract partition image to disk
//*****************************************
void MainWindow::Menu_Part_Extract() {
  
int np=partlist->currentRow();
QString filename;
QString str;
FILE* out;

filename.sprintf("%02i-%08x-%s.bin",np,ptable->code(np),ptable->name(np));
filename=QFileDialog::getSaveFileName(this,"Extracted file name",filename,"image (*.bin);;All files (*.*)");
if (filename.isEmpty()) return;
out=fopen(filename.toLocal8Bit().data(),"w");
if (out == 0) {
  QMessageBox::critical(0,"Error","Error opening file");
  return;
}
fwrite(ptable->iptr(np),1,ptable->psize(np),out);
fclose(out);
}


//*****************************************
//*  Replace partition image 
//*****************************************
void MainWindow::Menu_Part_Replace() {

int np=partlist->currentRow();
QString filename;
QString str;
char fileselector[100];
FILE* in;
// Select appropriate file extensions
enum parttypes ptype=ptable->ptype(np);
printf("\n ptype = %i",ptype);
switch (ptype) {
  case part_cpio:
    strcpy(fileselector,"CPIO archive (*.cpio)");
    break;
      
  case part_nvram:
    strcpy(fileselector,"NVDLOAD image (*.nvd)");
    break;

  case part_iso:
    strcpy(fileselector,"ISO image (*.iso)");
    break;

  case part_ptable:
    strcpy(fileselector,"Partition table (*.ptable)");
    break;

  case part_bin:
  default:  
    strcpy(fileselector,"image (*.bin)");
    break;
   
    
}
strcat(fileselector,";;All files (*.*)");
      

filename.sprintf("%02i-%08x-%s.bin",np,ptable->code(np),ptable->name(np));
filename=QFileDialog::getOpenFileName(this,"Имя файла с образом раздела",filename,fileselector);
if (filename.isEmpty()) return;
in=fopen(filename.toLocal8Bit(),"r");
if (in == 0) {
  QMessageBox::critical(0,"Ошибка","Ошибка открытия файла");
  printf("\n file %s",filename.toLocal8Bit().data());
  return;
}
ptable->loadimage(np,in);
hrow=-1; // предыдущие данные НЕ СОХРАНЯТЬ !!!!
SelectPart();
  
}

//*****************************************
//*  Delete partition 
//*****************************************
void MainWindow::Menu_Part_Delete() {

int32_t ci=partlist->currentRow(); 

if (ptable->index() == 1) return; // cannot delete last partition
removeEditor(); // remove current editor
ptable->delpart(ci);
regenerate_partlist();
if (ci< (ptable->index()-1)) partlist->setCurrentRow(ci);
else partlist->setCurrentRow(ptable->index()-1);
SelectPart();  
}


//*****************************************
//*  Move partition up 
//*****************************************
void MainWindow::Menu_Part_MoveUp() {
  
int32_t ci=partlist->currentRow(); 
  
ptable->moveup(ci);
regenerate_partlist();
if (ci>0) partlist->setCurrentRow(ci-1);
else partlist->setCurrentRow(0);
}

//*****************************************
//*  Move partition down
//*****************************************
void MainWindow::Menu_Part_MoveDown() {

int32_t ci=partlist->currentRow(); 
  
ptable->movedown(ci);
regenerate_partlist();
if (ci<(ptable->index()-1)) partlist->setCurrentRow(ci+1);
else partlist->setCurrentRow(ptable->index()-1);
}


//********************************************
// Enable header field editing
//********************************************
void MainWindow::Menu_Part_EditHeader() {
  
  
Platform_input->setReadOnly(0);
Date_input->setReadOnly(0);
Time_input->setReadOnly(0);
Version_input->setReadOnly(0);
}


//********************************************
//* Set current partition modification date
//********************************************
void MainWindow::set_date() {

QString str;
time_t actime=time(0);
struct tm* mtime=localtime(&actime);

str.sprintf("%4i.%02i.%02i",mtime->tm_year+1900,mtime->tm_mon+1,mtime->tm_mday);
Date_input->setText(str);
Date_input->setModified(true);

str.sprintf("%02i:%02i:%02i",mtime->tm_hour,mtime->tm_min,mtime->tm_sec);
Time_input->setText(str);
Time_input->setModified(true);
}
//******************************************************
//* Copy string with trailing space trimming 
//******************************************************
void fieldcopy(uint8_t* to,QByteArray from, uint32_t len) {
  
uint32_t i;

for (i=0;i<len;i++) {
  if (from.at(i) != ' ') to[i]=from.at(i);
  else break;
}  
if (i != len) {
  for(;i<len;i++) to[i]=0;
}  
}

//********************************************
//* Write header edit areas
//********************************************
void MainWindow::HeaderChanged() {

int32_t ci=hrow; // partition list row corresponding to header 
uint32_t code;
QMessageBox::StandardButton reply;

// check if anything has changed
if (!(
     (Platform_input->isModified()) ||
     (Date_input->isModified()) ||
     (Time_input->isModified()) ||
     (Version_input->isModified()) ||
     (pcode -> isModified())
   )) return;  

reply=QMessageBox::warning(this,"Write header","Partition header has been modified, save?",QMessageBox::Ok | QMessageBox::Cancel);
if (reply != QMessageBox::Ok) return;
if (Platform_input->isModified())  fieldcopy((uint8_t*)ptable->hptr(ci)->unlock,Platform_input->text().toLocal8Bit(),8);
if (Date_input->isModified())  fieldcopy((uint8_t*)ptable->hptr(ci)->date,Date_input->text().toLocal8Bit(),16);
if (Time_input->isModified())  fieldcopy((uint8_t*)ptable->hptr(ci)->time,Time_input->text().toLocal8Bit(),16);
if (Version_input->isModified())  fieldcopy((uint8_t*)ptable->hptr(ci)->version,Version_input->text().toLocal8Bit(),32);
if (pcode -> isModified()) {
  sscanf(pcode->text().toLocal8Bit(),"%x",&code);
  ptable->hptr(ci)->code=code<<16;
}
ptable->calc_hd_crc16(ci);
}

//********************************************
//* Write modified data
//********************************************
void MainWindow::DataChanged() {

char* tdata;  
QByteArray hexcup;
QMessageBox::StandardButton reply;

//  Modified oeminfo partition  
if (oemedit != 0) {
  tdata=new char[ptable->psize(hrow)];
  bzero(tdata,ptable->psize(hrow));
  fieldcopy((uint8_t*)tdata,oemedit->text().toLocal8Bit(),oemedit->text().size());
  if (memcmp(tdata,ptable->iptr(hrow),ptable->psize(hrow)) != 0) {
    reply=QMessageBox::warning(this,"Write partition","Partition content has been modified, save?",QMessageBox::Ok | QMessageBox::Cancel);
    if (reply == QMessageBox::Ok) ptable->replace(hrow,(uint8_t*)tdata,ptable->psize(hrow));
  }
  delete [] tdata;
  return;
}
// Modified partition handled by hex-editor
if (hexedit != 0) {
  tdata=new char[ptable->psize(hrow)];
  hexcup=hexedit->dhex->data();
  memcpy(tdata,hexcup.data(),ptable->psize(hrow));
  if (memcmp(tdata,ptable->iptr(hrow),ptable->psize(hrow)) != 0) {
    reply=QMessageBox::warning(this,"Write partition","Partition content has been modified, save?",QMessageBox::Ok | QMessageBox::Cancel);
    if (reply == QMessageBox::Ok) ptable->replace(hrow,(uint8_t*)tdata,ptable->psize(hrow));
  }
  delete [] tdata;
  return;
}  
  
}  

//********************************************
// Disable header field editing
//********************************************
void MainWindow::Disable_EditHeader() {
  
  
Platform_input->setReadOnly(0);
Date_input->setReadOnly(0);
Time_input->setReadOnly(0);
Version_input->setReadOnly(0);
}

//********************************************
// Start flasher dialog
//********************************************
void MainWindow::Start_Flasher() {

if (PortSelector->count() == 0) {
   QMessageBox::critical(0,"Ошибка","Не найдены последовательне порты");
   return;
}
  
flasher();
}

//********************************************
//  Reboot modem
//********************************************
void MainWindow::Reboot_modem() {

if (PortSelector->count() == 0) {
   QMessageBox::critical(0,"Error","No serial ports found");
   return;
}
open_port();
modem_reboot();  
close_port();
QMessageBox::information(0,"OK","Reboot command sent to modem");
}


//********************************************
// Start usb-bootloader
//********************************************
void MainWindow::usbdload() {

 
if (PortSelector->count() == 0) {
   QMessageBox::critical(0,"Ошибка","Не найдены последовательне порты");
   return;
}
usbload();
}


//********************************************
//* Set modification flag
//********************************************
void MainWindow::setModified() {

if (modified) return;  
modified=true;
// add asterisk to title
QString str=windowTitle();
str.append(" *");
setWindowTitle(str);

}

//********************************************************
//* Static function to set modification flag
//********************************************************
void set_modified() { mw->setModified(); }


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@222

int main(int argc, char* argv[]) {
  
QApplication app(argc, argv);
QString deffile;
QCoreApplication::setApplicationName("Qt linux huawei flasher");
QCoreApplication::setApplicationVersion("3.0");
app.setOrganizationName("forth32");
MainWindow* mwin;

QCommandLineParser parser;

parser.setApplicationDescription("Program for flashing and recovering devices based on Hisilicon Balong v7 chipset");
parser.addHelpOption();
parser.addPositionalArgument("firmware", "Firmware file");

parser.process(app);    
QStringList args = parser.positionalArguments();

if(args.size() > 0) deffile=args[0]; 

mwin = new  MainWindow(deffile);
mw=mwin;
mwin->setAttribute(Qt::WA_DeleteOnClose);
mwin->show();
return app.exec();
}
