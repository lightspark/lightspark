#ifdef WIN32
#define PACKED
#pragma pack(push,1)
#else
#define PACKED __attribute__ ((__packed__))
#endif