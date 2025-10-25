// 
//  Save firmware file to disk
// 
#include <QtWidgets>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "fwsave.h"
#include "ptable.h"
#include "signver.h"

extern QString fwfilename;
  
//****************************************************************
//* Procedure to save firmware image to new file
//****************************************************************
void fw_saver(bool newname, bool zflag) {

int32_t i;  
FILE* out;
uint8_t hdr[92];
uint32_t percent;
char fname[200];
int dlcode;

if (newname || fwfilename.isEmpty())  { 
  // select new file name
  QString fn=fwfilename;

  fn=QFileDialog::getSaveFileName(0,"File name",fn,"firmware (*.fw);;All files (*.*)");
  if (fn.isEmpty()) return;
  fwfilename=fn;  
}

strcpy(fname,fwfilename.toLocal8Bit().data());

dlcode=dload_id&7;

out=fopen(fname,"w");
if (out == 0) {
    QMessageBox::critical(0,"Error","Error creating file");
    return;
}

// write header - upgrade state
bzero(hdr,sizeof(hdr));
// extract firmware type code
hdr[0]=dlcode;
if (signlen != -1) hdr[0]|=0x8;
fwrite(hdr,1,sizeof(hdr),out);

// Create progress bar window
QWidget* pb=new QWidget();
QVBoxLayout* plm=new QVBoxLayout(pb);

QLabel* lb = new QLabel("Saving partitions",pb);
QFont font;
font.setPointSize(14);
font.setBold(true);
font.setWeight(75);
lb->setFont(font);
plm->addWidget(lb);

QProgressBar* fbar = new QProgressBar(pb);
fbar->setValue(0);
plm->addWidget(fbar);

pb->show();

// write images of all partitions
for(i=0;i<ptable->index();i++) {
  ptable->save_part(i,out,zflag);
  percent=(i+1)*100/(ptable->index());
  fbar->setValue(percent);
  QCoreApplication::processEvents();
}
delete pb;  

fclose(out);
}  
    
