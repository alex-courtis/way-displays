#ifndef SERVER_H
#define SERVER_H

// start the server, tearing all down when done
int server(char *cfg_path);

// find and read a config file into g_cfg
void server_load_cfg(void);

// reload g_cfg from its path
void server_reload_cfg(void);

#endif // SERVER_H

