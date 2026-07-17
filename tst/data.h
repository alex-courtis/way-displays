#ifndef DATA_H
#define DATA_H

#include "enum.h"
#include "pslist.h"

// head ppmap keys
static void *H0 = "H0";
static void *H1 = "H1";
static void *H2 = "H2";
static void *H3 = "H3";
static void *H4 = "H4";
static void *H5 = "H5";
static void *H6 = "H6";
static void *H7 = "H7";
static void *H8 = "H8";
static void *H9 = "H9";

// mode ppmap keys
static void *MC = "MC";
static void *MD = "MD";
static void *MP = "MP";
static void *MF = "MF";

static void *M0 = "M0";
static void *M1 = "M1";
static void *M2 = "M2";
static void *M3 = "M3";
static void *M4 = "M4";
static void *M5 = "M5";
static void *M6 = "M6";
static void *M7 = "M7";
static void *M8 = "M8";
static void *M9 = "M9";
static void *M10 = "M10";

struct Cfg *cfg_all(void);

struct IpcOperation *ipc_response(void);

// add a LogCapLine to log_cap_lines
void log_cap_line_append(enum LogThreshold threshold, const char *line, struct Pslist **log_cap_lines);

#endif // DATA_H
