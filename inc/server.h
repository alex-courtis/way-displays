#ifndef SERVER_H
#define SERVER_H

extern struct Displ *displ;
extern struct Cfg *cfg;
extern struct Lid *lid;

int server(char *cfg_path);

#endif // SERVER_H

