# qhuaweiflash
Graphical utility for flashing HUAWEI modems and routers and editing firmware files

This utility is designed for:

- Flashing HUAWEI modems that support a flashing protocol similar to that used in Balong V7 modems. Full support for digital signatures of firmwares is implemented.
- Editing firmware images. It is possible to view, add, delete, modify individual partitions, and change partition headers. 
Editing partition images in HEX-code and, partially, in formatted mode (if the partition has some meaningful format) is implemented.
- Loading usbloader bootloaders into the modem with patches applied.

The utility is built on the Qt graphics package, and is a GUI version of the balong_flash, balong-usbload utilities, and also a firmware editor.

To build the utility use the commands:

qmake

make
