#include <QtWidgets>

//****************************************************************
//* Dialog window class
//****************************************************************
class usbldialog: public QDialog {

Q_OBJECT
public:
  QLineEdit* fname=0;
  QLineEdit* ptfname=0;
  
  // constructor
  usbldialog(): QDialog(0){};

  // destructor
  ~usbldialog() {
    if (fname != 0) delete fname;
    if (ptfname != 0) delete ptfname;
  }
  
public slots: 
  void browse();
  void ptbrowse();
  void ptclear();
};  

//****************************************************************
// Bootloader header
//****************************************************************
struct lhead{
  uint32_t lmode;  // launch mode: 1 - direct start, 2 - through A-core restart
  uint32_t size;   // component size
  uint32_t adr;    // component memory load address
  uint32_t offset; // offset to component from file start
};

void usbload();