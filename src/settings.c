/*
 *	settings.c
 */

#include "ContactCards.h"

static char			*alternate_config = NULL;
static char			*version = NULL;
static char			*verbose = NULL;

static GOptionEntry entries[] =
{
	{ "config", 'c', 0, G_OPTION_ARG_FILENAME, &alternate_config, "Alternate configuration directory", NULL },
	{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Show debug stuff", NULL },
	{ "version", 'V', 0, G_OPTION_ARG_NONE, &version, "Show version", NULL },
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

ContactCards_app_t *parseCmdLine(int *argc, char **argv[]){
	printfunc(__func__);

	GError				*error = NULL;
	GOptionContext		*context;
	ContactCards_app_t	*app;

	context = g_option_context_new(_("ContactCards"));
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, argc, argv, &error);
	g_option_context_free(context);

	if(error){
		dbgCC("%s\n", error->message);
		exit(EXIT_FAILURE);
	}

	if (version){
		printf("ContactCards %s\n", VERSION);
		exit(EXIT_SUCCESS);
	}

	app = g_new(ContactCards_app_t, 1);

	if(alternate_config){
		app->configdir = alternate_config;
	}else {
		app->configdir = g_build_filename(g_get_user_config_dir(), "contactcards", NULL);
	}

	return app;
}

static void checkConfigDir(char *dir){
	printfunc(__func__);

	if (!g_file_test(dir, G_FILE_TEST_EXISTS)){
		int			ret = 0;
		dbgCC("[%s] configuration directory doesn't exist yet\n", __func__);

		if (dir == NULL || strlen(dir) == 0)
			exit(EXIT_FAILURE);

		ret = g_mkdir_with_parents(dir, 0700);
		if(ret)
			dbgCC("[%s] failed to create the configuration directory with error %d\n", __func__, ret);
	}
}

void checkAndSetConfig(ContactCards_app_t *app){
	printfunc(__func__);

	checkConfigDir(app->configdir);
}
