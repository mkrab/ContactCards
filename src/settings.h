/*
 *	settings.h
 */

#ifndef	settings_H
#define	settings_H

/**
 * struct ContactCards_app -struct for settings
 * It's a struct to be flexible for the future.
 */
typedef struct ContactCards_app {
	char		*configdir;
} ContactCards_app_t;

extern void checkAndSetConfig(ContactCards_app_t *app);
extern ContactCards_app_t *parseCmdLine(int *argc, char **argv[]);

#endif	/*	settings_H		*/
