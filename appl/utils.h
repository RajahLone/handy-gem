//
//  utils.h
//

int16_t fsel_cart(char *path, char *name, int16_t *button, const char *label);

int16_t has_extension(const char *filename, const char *ext);

int16_t get_current_dir(int16_t drive, char *path);

int16_t get_cookie(const uint32_t cookie_id, uint32_t *cookie_val);

uint16_t Mxmask(void);
