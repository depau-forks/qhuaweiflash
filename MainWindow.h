#include <QtWidgets>
#include "ptable.h"
#include "hexeditor.h"
#include "kerneledit.h"
#include "nvdedit.h"
#include "cpio.h"


//******************************************************************************
//* Main window class
//******************************************************************************
class MainWindow: public QMainWindow {
  
Q_OBJECT

QTableWidget* ptedit=0; // partition table editor 
QLineEdit* oemedit=0;   // oeminfo partition editor
QLabel* label=0;
QSpacerItem* spacer=0; // spacer for short editor forms
hexeditor* hexedit=0;
kerneledit* kedit=0;  // kernel partition editor
nvdedit* nvedit=0;  // nvram partition editor
cpioedit* cpio=0;   // filesystem partition editor

bool modified=false;

// Settings storage
QSettings* config;

int hrow=-1;   // partition list row corresponding to the current header
int structure_mode_save=-1; // previous state of the dump-format toggle

enum parttypes partmode=part_bin;

public:

MainWindow(QString startfile);
virtual ~MainWindow(); 

// Base widget - vertical splitter
QSplitter *centralwidget;

// Main window icon
QIcon icon;

// Menu handlers
QAction *fileopen;
QAction *fileappend;
QAction *part_store;
QAction *part_extract;
QAction *part_replace;
QAction *file_exit;
QAction *filesave;
QAction *MoveUp;
QAction *MoveDown;
QAction *Delete;
QAction *part_copy_header;
QAction *Menu_Oper_flash;
QAction *Menu_Oper_USBDload;
QAction *Menu_Oper_Reboot;
QAction *Menu_Oper_signinfo;

// Interface elements

// Header editor elements
QWidget* hdrpanel; // root widget
QVBoxLayout* vlhdr; // main vertical layout
QLabel* hdlbl1;
QListWidget *partlist; // partition list
QLabel* hdlbl2;
QFormLayout* lphdr;    // header field editors
QLineEdit *Date_input;
QLineEdit *Time_input;
QToolButton *setdate;
QComboBox* dload_id_selector;
QCheckBox* zflag_selector;
QLineEdit *Version_input;
QLineEdit *pcode;
QLabel* hdlbl3;
QLineEdit *Platform_input;

// Partition editor elements
QWidget* edpanel;
QVBoxLayout* EditorLayout;

// Raw-formatted buttons
QBoxLayout* laymode;
QGroupBox *modebuttons;
QRadioButton *dump_mode;
QRadioButton *structure_mode;

// separator line
QFrame* hframe;

// Main menu
QMenuBar *menubar;
QMenu *menu_file;
QMenu *menu_oper;
QMenu *menu_part;

// Status bar
QStatusBar* statusbar;
// Port selection
QLabel* plbl;
QComboBox *PortSelector;
QToolButton *RefreshPorts;

void open_recent(int n);
void settitle() {setWindowTitle("Huawei firmware editor/flasher");} 
void ask_save();
void removeEditor();

// Main menu handler slots
public slots: 
void  SelectFwFile();  // file selection
void  AppendFwFile();  // file append
void  SaveFwFile();    // write complete image to disk
void  save_as();
void  OpenFwFile(QString filename); // open firmware file
void SelectPart();     // firmware partition selection
void Menu_Part_Store();
void Menu_Part_Extract();  
void Menu_Part_Replace();
void Menu_Part_Delete();
void Menu_Part_MoveUp();
void Menu_Part_MoveDown();
void Terminate() { QCoreApplication::quit(); }
void regenerate_partlist();
void Menu_Part_EditHeader();  
void Disable_EditHeader();
void HeaderChanged();
void DataChanged();
void Start_Flasher();  
void Reboot_modem();  
void usbdload();
void find_ports();  
void EnableMenu();
void set_date();
void ShowSignInfo();
void HeadCopy();
void open_recent_file();

void setModified();  

};

// Class-independent handlers
void head_copy();

extern MainWindow* mw;
