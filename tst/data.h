#ifndef DATA_H
#define DATA_H

#include "enum.h"
#include "pslist.h"

// head ppmap keys
extern void *H0;
extern void *H1;
extern void *H2;
extern void *H3;
extern void *H4;
extern void *H5;
extern void *H6;
extern void *H7;
extern void *H8;
extern void *H9;

// mode ppmap keys
extern void *MC;
extern void *MD;
extern void *MP;
extern void *MF;
extern void *MR;

extern void *M0;
extern void *M1;
extern void *M2;
extern void *M3;
extern void *M4;
extern void *M5;
extern void *M6;
extern void *M7;
extern void *M8;
extern void *M9;
extern void *M10;

struct Cfg *cfg_all(void);

struct IpcOperation *ipc_response(void);

// add a LogCapLine to log_cap_lines
void log_cap_line_append(enum LogThreshold threshold, const char *line, struct Pslist **log_cap_lines);

#endif // DATA_H
