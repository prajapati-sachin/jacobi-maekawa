typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
//Signal handler function
typedef void (*signal_handler)(void *sent_message);

#define MSGSIZE 8
//max no. of message a process can hold at once
#define NUM_MSG 32
//max no. of signal a process can hold at once
#define MAX_SIG 5