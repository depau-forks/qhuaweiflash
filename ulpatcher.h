
// Structure describing signature and patch position
struct defpatch {
 const uint8_t* sig; // signature
 uint32_t sigsize; // signature length
 int32_t poffset;  // offset to patch point from end of signature
};



//***********************************************************************
//* Search for signature and apply patch
//***********************************************************************
uint32_t patch(struct defpatch fp, uint8_t* buf, uint32_t fsize, uint32_t ptype);

//****************************************************
//* Patch procedures for different chipsets and tasks
//****************************************************

uint32_t pv7r22 (uint8_t* buf, uint32_t fsize);
uint32_t pv7r22_2 (uint8_t* buf, uint32_t fsize);
uint32_t pv7r2 (uint8_t* buf, uint32_t fsize);
uint32_t pv7r11 (uint8_t* buf, uint32_t fsize);
uint32_t pv7r1 (uint8_t* buf, uint32_t fsize);
uint32_t perasebad (uint8_t* buf, uint32_t fsize);

