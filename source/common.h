#define printf(fmt, ...) printf("%s: " fmt, __func__, ##__VA_ARGS__);
