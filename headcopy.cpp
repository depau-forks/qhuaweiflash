// 
//  Copy partition headers
// 
#include <QtWidgets>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "ptable.h"

//**************************************************
//* Copy partition header to another partition
//**************************************************
void head_copy() {

uint32_t i,res;  
char str[100];
int src,dst;

QDialog* qd=new QDialog;  
QGridLayout* lm=new QGridLayout(qd);

QFont font;
font.setPointSize(14);
font.setBold(true);
font.setWeight(75);

QLabel* label1 = new QLabel("Source",qd);
label1->setFont(font);
lm->addWidget(label1,0,0);

QLabel* label2 = new QLabel("Destination",qd);
label2->setFont(font);
lm->addWidget(label2,0,1);

QComboBox* from=new QComboBox(qd);
lm->addWidget(from,1,0);

QComboBox* to=new QComboBox(qd);
lm->addWidget(to,1,1);

QDialogButtonBox* buttonBox = new QDialogButtonBox(qd);
buttonBox->setOrientation(Qt::Horizontal);
buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
lm->addWidget(buttonBox,2,1);

QObject::connect(buttonBox, SIGNAL(accepted()), qd, SLOT(accept()));
QObject::connect(buttonBox, SIGNAL(rejected()), qd, SLOT(reject()));

// create copy source list
for(i=0;i<ptable->index();i++) {
  sprintf(str,"%02i %s",i,ptable->name(i));
  from->insertItem(i,str);
}
// create copy destination list
to->insertItem(0,"all partitions");
for(i=0;i<ptable->index();i++) {
  sprintf(str,"%02i %s",i,ptable->name(i));
  to->insertItem(i+1,str);
}
from->setCurrentIndex(0); 
to->setCurrentIndex(0); 

res=qd->exec();

src=from->currentIndex();
dst=to->currentIndex()-1;

delete qd;
if (res !=  QDialog::Accepted) return;

// operator confirmed execution

// partition header description structure
struct __attribute__ ((__packed__)) pheader {
 uint32_t magic;    //   0xa55aaa55
 uint32_t hdsize;   // header size
 uint32_t hdversion; // header version
 uint8_t unlock[8]; // platform
 uint32_t code;     // partition type
 uint32_t psize;    // data field size
 uint8_t date[16];
 uint8_t time[16];  // firmware build date-time
 uint8_t version[32];   // firmware version
 uint16_t crc;   // header CRC
 uint32_t blocksize;  // firmware image CRC block size
}; 


for(i=0;i<ptable->index();i++) {
  if ((i == dst) || (dst == -1)) {
    memcpy(ptable->hptr(i)->date,ptable->hptr(src)->date,16);
    memcpy(ptable->hptr(i)->time,ptable->hptr(src)->time,16);
    memcpy(ptable->hptr(i)->version,ptable->hptr(src)->version,32);
  }
}
}  
