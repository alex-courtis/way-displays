#ifndef SERVER_H
#define SERVER_H

int server(char *cfg_path);

//
// visible for testing
//
void load_cfg(void);
void reload_cfg(void);

#endif // SERVER_H

