// #define LOGGING_OFF

#ifdef LOGGING_OFF
#define _DEBUG(msg) (void)(msg)
#define _ERROR(msg) (void)(msg)
#else
#define _DEBUG(msg) std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] " << msg << endl
#define _ERROR(msg) std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] " \
    "ERROR: "<< msg << endl
#endif

#include <iostream>
