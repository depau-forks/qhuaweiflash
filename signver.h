int32_t send_signver();
int32_t search_sign();

// Current digital signature parameters

extern int32_t signlen;  // signature length
// Public key hash for ^signver
extern char signver_hash[100];


