/*
 *	gui.c
 */

#include "ContactCards.h"

void guiRun(sqlite3 *ptr){
	printfunc(__func__);

	fillList(ptr, 1, 0, addressbookList);
	gtk_main();
}

void guiExit(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GSList			*cleanUpList = data;

	gtk_main_quit();
	g_slist_free_full(cleanUpList, g_free);
}


void guiKeyHandler(GtkWidget *gui, GdkEventKey *event, gpointer data){
	printfunc(__func__);

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		guiExit(gui, data);
	}
}

void dialogKeyHandler(GtkDialog *widget, GdkEventKey *event, gpointer data){
	printfunc(__func__);

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		gtk_dialog_response(widget, GTK_RESPONSE_DELETE_EVENT);
	}
}

static void dialogAbout(void){
	printfunc(__func__);

	gtk_show_about_dialog(NULL,
		"title", _("About ContactCards"),
		"program-name", "ContactCards",
		"comments", _("Address book written in C using DAV"),
		"website", "https://www.der-flo.net/ContactCards.html",
		"license", "GNU General Public License, version 2\nhttp://www.gnu.org/licenses/old-licenses/gpl-2.0.html",
		"version", VERSION,
		NULL);
}

static void selBook(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	char				*selText;
	int					selID;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;

	ptr = data->db;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, TEXT_COLUMN, &selText, ID_COLUMN, &selID,  -1);
		dbgCC("[%s] %d\n",__func__, selID);
		fillList(ptr, 2, selID, contactList);
		g_free(selText);
	}
}

void prefServerDelete(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers;

	ptr = data->db;
	buffers = data->element2;

	dbgCC("[%s] %d\n", __func__, buffers->srvID);

	dbRemoveItem(ptr, "cardServer", 2, "", "", "serverID", buffers->srvID);
	cleanUpRequest(ptr, buffers->srvID, 0);
	fillList(ptr, 3, 0, buffers->srvPrefList);
	fillList(ptr, 1, 0, addressbookList);
}

void prefServerSave(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers;

	ptr = data->db;
	buffers = data->element2;

	updateServerDetails(ptr, buffers->srvID,
						gtk_entry_buffer_get_text(buffers->descBuf), gtk_entry_buffer_get_text(buffers->urlBuf), gtk_entry_buffer_get_text(buffers->userBuf), gtk_entry_buffer_get_text(buffers->passwdBuf),
						gtk_switch_get_active(GTK_SWITCH(buffers->resSel)),
						gtk_switch_get_active(GTK_SWITCH(buffers->certSel)));
}

void prefExportCert(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	sqlite3					*ptr;
	ContactCards_trans_t	*data = trans;
	ContactCards_pref_t		*buffers;
	GtkWidget					*dirChooser;
	int							result;
	char						*path = NULL;

	ptr = data->db;
	buffers = data->element2;


	dirChooser = gtk_file_chooser_dialog_new(_("Export Certificate"), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Export"), GTK_RESPONSE_ACCEPT, NULL);

	g_signal_connect(G_OBJECT(dirChooser), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	result = gtk_dialog_run(GTK_DIALOG(dirChooser));

	switch(result){
		case GTK_RESPONSE_ACCEPT:
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dirChooser));
			exportCert(ptr, path, buffers->srvID);
			g_free(path);
			break;
		default:
			break;
	}
	gtk_widget_destroy(dirChooser);
}

void prefServerSelect(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkWidget			*prefFrame;
	int					selID;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers;
	char				*frameTitle = NULL, *user = NULL, *passwd = NULL;
	char				*issued, *issuer, *url = NULL;
	int					isOAuth;
	gboolean			res = 0;

	ptr = data->db;
	prefFrame = data->element;
	buffers = data->element2;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, ID_COLUMN, &selID,  -1);
		dbgCC("[%s] %d\n",__func__, selID);
		frameTitle = getSingleChar(ptr, "cardServer", "desc", 1, "serverID", selID, "", "", "", "", "", 0);
		if (frameTitle == NULL) return;
		gtk_frame_set_label(GTK_FRAME(prefFrame), frameTitle);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->descBuf), frameTitle, -1);

		buffers->srvID = selID;

		user = getSingleChar(ptr, "cardServer", "user", 1, "serverID", selID, "", "", "", "", "", 0);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->userBuf), user, -1);

		isOAuth = getSingleInt(ptr, "cardServer", "isOAuth", 1, "serverID", selID, "", "");
		if(!isOAuth){
			passwd = getSingleChar(ptr, "cardServer", "passwd", 1, "serverID", selID, "", "", "", "", "", 0);
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), passwd, -1);
		} else {
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), "", -1);
		}

		url = getSingleChar(ptr, "cardServer", "srvUrl", 1, "serverID", selID, "", "", "", "", "", 0);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->urlBuf), url, -1);

		res = getSingleInt(ptr, "cardServer", "resources", 1, "serverID", selID, "", "");
		gtk_switch_set_active(GTK_SWITCH(buffers->resSel), res);

		issued = getSingleChar(ptr, "certs", "issued", 1, "serverID", selID, "", "", "", "", "", 0);
		if(issued == NULL) issued = "";
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuedBuf), issued, -1);

		issuer = getSingleChar(ptr, "certs", "issuer", 1, "serverID", selID, "", "", "", "", "", 0);
		if(issuer == NULL) issuer = "";
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuerBuf), issuer, -1);

		res = getSingleInt(ptr, "certs", "trustFlag", 1, "serverID", selID, "", "");
		if(res == ContactCards_DIGEST_TRUSTED){
			gtk_switch_set_active(GTK_SWITCH(buffers->certSel), TRUE);
		} else {
			gtk_switch_set_active(GTK_SWITCH(buffers->certSel), FALSE);
		}
	}
}

static void selContact(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkTextBuffer		*dataBuffer;
	int					selID;
	char				*vData = NULL;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;

	ptr = data->db;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, ID_COLUMN, &selID,  -1);
		dbgCC("[%s] %d\n",__func__, selID);
		vData = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", selID, "", "", "", "", "", 0);
		if(vData == NULL) vData = "";
		dataBuffer = gtk_text_view_get_buffer(data->element);
		gtk_text_view_set_editable(data->element, FALSE);
		gtk_text_view_set_wrap_mode(data->element, GTK_WRAP_CHAR);
		gtk_text_buffer_set_text(dataBuffer, vData, -1);
	}
}

void listFlush(GtkWidget *list){
	printfunc(__func__);

	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeIter  iter;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (list)));
	if(store == NULL) return;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
	if (gtk_tree_model_get_iter_first(model, &iter) == FALSE){
		return;
	}
	gtk_list_store_clear(store);
}

void listAppend(GtkWidget *list, gchar *text, guint id) {
	printfunc(__func__);

	GtkListStore		*store;
	GtkTreeIter 		iter;

	store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(list)));

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, TEXT_COLUMN, text, ID_COLUMN, id, -1);
}

void listInit(GtkWidget *list){
	printfunc(__func__);

	GtkListStore		*store;
	GtkTreeViewColumn	*column;
	GtkCellRenderer		*renderer;
	GtkTreeSortable		*sortable;


	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "text", TEXT_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_UINT);

	sortable = GTK_TREE_SORTABLE(store);

	gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));

	g_object_unref(store);
}

static void *syncOneServer(void *trans){
	printfunc(__func__);

	int					serverID = 0;
	int					isOAuth = 0;
	ne_session 			*sess = NULL;
	ContactCards_trans_t		*data = trans;
	sqlite3 			*ptr = data->db;
	GtkWidget			*statusBar;
	char				*srv = NULL;
	char				*msg = NULL;
	int					ctxID;

	g_mutex_lock(&mutex);

	serverID = GPOINTER_TO_INT(data->element);
	statusBar = data->element2;

	srv = getSingleChar(ptr, "cardServer", "desc", 1, "serverID", serverID, "", "", "", "", "", 0);
	msg = g_strconcat(_("syncing "), srv, NULL);
	ctxID = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "info");
	gtk_statusbar_push(GTK_STATUSBAR(statusBar), ctxID, msg);

	isOAuth = getSingleInt(ptr, "cardServer", "isOAuth", 1, "serverID", serverID, "", "");

	if(isOAuth){
		char				*oAuthGrant = NULL;
		char				*oAuthToken = NULL;
		char				*oAuthRefresh = NULL;
		int					oAuthEntity = 0;

		oAuthGrant = getSingleChar(ptr, "cardServer", "oAuthAccessGrant", 1, "serverID", serverID, "", "", "", "", "", 0);
		oAuthEntity = getSingleInt(ptr, "cardServer", "oAuthType", 1, "serverID", serverID, "", "");
		dbgCC("[%s] connecting to a oAuth-Server\n", __func__);
		if(strlen(oAuthGrant) == 1){
			char		*newuser = NULL;
			newuser = getSingleChar(ptr, "cardServer", "user", 1, "serverID", serverID, "", "", "", "", "", 0);
			dialogRequestGrant(ptr, serverID, oAuthEntity, newuser);
		} else {
			dbgCC("[%s] there is already a grant\n", __func__);
		}
		oAuthRefresh = getSingleChar(ptr, "cardServer", "oAuthRefreshToken", 1, "serverID", serverID, "", "", "", "", "", 0);
		if(strlen(oAuthRefresh) == 1){
			dbgCC("[%s] there is no refresh_token\n", __func__);
			oAuthAccess(ptr, serverID, oAuthEntity, DAV_REQ_GET_TOKEN);
		} else {
			dbgCC("[%s] there is already a refresh_token\n", __func__);
			oAuthAccess(ptr, serverID, oAuthEntity, DAV_REQ_GET_REFRESH);
		}
		oAuthToken = getSingleChar(ptr, "cardServer", "oAuthAccessToken", 1, "serverID", serverID, "", "", "", "", "", 0);
		if(strlen(oAuthToken) == 1){
			dbgCC("[%s] there is no oAuthToken\n", __func__);
			g_free(data);
			g_mutex_unlock(&mutex);
			return NULL;
		}
	}

	sess = serverConnect(data);
	syncContacts(ptr, sess, serverID);
	serverDisconnect(sess, ptr);

	gtk_statusbar_pop(GTK_STATUSBAR(statusBar), ctxID);

	g_free(data);
	g_mutex_unlock(&mutex);

	return NULL;
}

static void dialogExportContacts(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	sqlite3						*ptr;
	ContactCards_trans_t		*data = trans;
	GtkWidget					*dirChooser;
	int							result;
	char						*path = NULL;

	ptr = data->db;

	dirChooser = gtk_file_chooser_dialog_new(_("Export Contacts"), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);

	g_signal_connect(G_OBJECT(dirChooser), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	result = gtk_dialog_run(GTK_DIALOG(dirChooser));

	switch(result){
		case GTK_RESPONSE_ACCEPT:
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dirChooser));
			exportContacts(ptr, path);
			g_free(path);
			break;
		default:
			break;
	}
	gtk_widget_destroy(dirChooser);
}

static void syncServer(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	sqlite3						*ptr;
	ContactCards_trans_t		*data = trans;
	GSList						*retList;
	GtkWidget					*statusBar;
	GError		 				*error = NULL;
	ContactCards_trans_t		*buff = NULL;

	ptr = data->db;
	statusBar = data->element;

	if(selectedSrv != 0){
		buff = g_new(ContactCards_trans_t, 1);
		buff->db = ptr;
		buff->element = GINT_TO_POINTER(selectedSrv);
		buff->element2 = statusBar;
		g_thread_try_new("syncingServer", syncOneServer, buff, &error);
		if(error){
			dbgCC("[%s] something has gone wrong with threads\n", __func__);
		}
	} else {

		retList = getListInt(ptr, "cardServer", "serverID", 0, "", 0, "", "");

		while(retList){
			GSList				*next = retList->next;
			int					serverID = GPOINTER_TO_INT(retList->data);
			if(serverID == 0){
				retList = next;
				continue;
			}
			buff = g_new(ContactCards_trans_t, 1);
			buff->db = ptr;
			buff->element = GINT_TO_POINTER(serverID);
			buff->element2 = statusBar;
			g_thread_try_new("syncingServer", syncOneServer, buff, &error);
			if(error){
				dbgCC("[%s] something has gone wrong with threads\n", __func__);
			}
			retList = next;
		}
		g_slist_free(retList);
	}
}

static void comboChanged(GtkComboBox *combo, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					id = 0;
	int 				counter = 0;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;

	ptr = data->db;

	if( gtk_combo_box_get_active_iter(combo, &iter)){
		model = gtk_combo_box_get_model(combo);
		gtk_tree_model_get( model, &iter, 1, &id, -1);
	}

	counter = countElements(ptr, "cardServer", 1, "serverID", id, "", "", "", "");

	if(counter == 1) {
		fillList(ptr, 1, id, addressbookList);
		selectedSrv = id;
	} else {
		fillList(ptr, 1, 0, addressbookList);
		selectedSrv = 0;
	}
}

void comboAppend(GtkListStore *store, gchar *text, guint id) {
	printfunc(__func__);

	GtkTreeIter 		iter;

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, TEXT_COLUMN, text, ID_COLUMN, id, -1);
}

void comboFlush(GtkListStore *store){
	printfunc(__func__);

	GtkTreeIter			iter;

	gtk_list_store_clear(store);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, TEXT_COLUMN, "All", ID_COLUMN, 0, -1);

}

GtkWidget *comboInit(sqlite3 *ptr){
	printfunc(__func__);

	GtkWidget			*combo;
	GtkCellLayout		*layout;
	GtkCellRenderer		*renderer;
	ContactCards_trans_t		*trans = NULL;

	comboList = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_UINT);

	fillCombo(ptr, comboList);

	combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(comboList));

	layout = GTK_CELL_LAYOUT(combo);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(layout, renderer, FALSE);
	gtk_cell_layout_set_attributes(layout, renderer, "text", 0, NULL);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	g_object_unref(comboList);

	trans = g_new(ContactCards_trans_t, 1);
	trans->db = ptr;

	g_signal_connect(combo, "changed", G_CALLBACK(comboChanged), trans);

	return combo;
}

void dialogRequestGrant(sqlite3 *ptr, int serverID, int entity, char *newuser){
	printfunc(__func__);

	GtkWidget			*dialog, *area, *box, *label;
	GtkWidget			*input, *button;
	GtkEntryBuffer		*grant;
	char				*newGrant = NULL;
	int					result;
	char				*uri = NULL;

	grant = gtk_entry_buffer_new(NULL, -1);

	dialog = gtk_dialog_new_with_buttons("Request for Grant", GTK_WINDOW(mainWindow), GTK_DIALOG_DESTROY_WITH_PARENT, _("_Save"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_CANCEL, NULL);

	uri = g_strdup("https://accounts.google.com/o/oauth2/auth?scope=https://www.googleapis.com/auth/carddav&redirect_uri=urn:ietf:wg:oauth:2.0:oob&response_type=code&client_id=741969998490.apps.googleusercontent.com");

	area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	button = gtk_link_button_new_with_label(uri, _("Request Grant"));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 2);
	label = gtk_label_new(_("Access Grant"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(grant);
	gtk_box_pack_start(GTK_BOX(box), input, FALSE, FALSE, 2);

	gtk_box_pack_start(GTK_BOX(area), box, FALSE, FALSE, 2);

	g_signal_connect(G_OBJECT(dialog), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));

	switch(result){
		case GTK_RESPONSE_ACCEPT:
			newGrant = g_strdup(gtk_entry_buffer_get_text(grant));
			g_strdelimit(newGrant, ",", '\n');
			g_strstrip(newGrant);
			if(strlen(newGrant) < 5){
				dbRemoveItem(ptr, "cardServer", 2, "", "", "serverID", serverID);
				break;
			}
			setSingleChar(ptr, "cardServer", "oAuthAccessGrant", newGrant, "serverID", serverID);
			g_free(newGrant);
			oAuthAccess(ptr, serverID, entity, DAV_REQ_GET_TOKEN);
			break;
		default:
			dbRemoveItem(ptr, "cardServer", 2, "", "", "serverID", serverID);
			break;
	}
	g_free(uri);
	gtk_widget_destroy(dialog);

}

void prefExit(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	g_free(data);
}

void prefKeyHandler(GtkWidget *window, GdkEventKey *event, gpointer data){
	printfunc(__func__);

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		gtk_widget_destroy(window);
	}
}

void prefWindow(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget			*prefWindow, *prefView, *prefFrame, *prefList;
	GtkWidget			*serverPrefList;
	GtkWidget			*vbox, *hbox;
	GtkWidget			*label, *input;
	GtkWidget			*saveBtn, *deleteBtn, *exportCertBtn;
	GtkWidget			*resSwitch, *digSwitch;
	GtkWidget			*sep;
	GtkEntryBuffer		*desc, *url, *user, *passwd;
	GtkEntryBuffer		*issued, *issuer;
	GtkTreeSelection	*serverSel;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers = NULL;

	desc = gtk_entry_buffer_new(NULL, -1);
	url = gtk_entry_buffer_new(NULL, -1);
	user = gtk_entry_buffer_new(NULL, -1);
	passwd = gtk_entry_buffer_new(NULL, -1);
	issued = gtk_entry_buffer_new(NULL, -1);
	issuer = gtk_entry_buffer_new(NULL, -1);

	buffers = g_new(ContactCards_pref_t, 1);

	ptr = data->db;

	serverPrefList = gtk_tree_view_new();
	listInit(serverPrefList);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(serverPrefList), FALSE);

	prefWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(prefWindow), _("Preferences"));
	gtk_window_resize(GTK_WINDOW(prefWindow), 640, 384);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(prefWindow), TRUE);

	prefView = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	prefList = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(prefList), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(prefList, 128, -1);

	prefFrame = gtk_frame_new(_("Settings"));
	data->element = prefFrame;
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Description"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(desc);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("URL"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(url);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("User"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(user);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Password"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(passwd);
	gtk_entry_set_visibility(GTK_ENTRY(input), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("AllAddressBooks"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	resSwitch = gtk_switch_new();
	gtk_box_pack_start(GTK_BOX(hbox), resSwitch, FALSE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	exportCertBtn = gtk_button_new_with_label(_("Export Certificate"));
	gtk_box_pack_start(GTK_BOX(hbox), exportCertBtn, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 10);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Certificate is issued for "));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(issued);
	gtk_editable_set_editable(GTK_EDITABLE(input), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Certificate issued by "));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(issuer);
	gtk_editable_set_editable(GTK_EDITABLE(input), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Trust Certificate?"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	digSwitch = gtk_switch_new();
	gtk_box_pack_start(GTK_BOX(hbox), digSwitch, FALSE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	deleteBtn = gtk_button_new_with_label(_("Delete Server"));
	gtk_box_pack_start(GTK_BOX(hbox), deleteBtn, FALSE, FALSE, 2);
	saveBtn = gtk_button_new_with_label(_("Save changes"));
	gtk_box_pack_start(GTK_BOX(hbox), saveBtn, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 10);

	gtk_container_add(GTK_CONTAINER(prefFrame), vbox);
	gtk_container_add(GTK_CONTAINER(prefList), serverPrefList);
	fillList(ptr, 3, 0 , serverPrefList);

	buffers->descBuf = desc;
	buffers->urlBuf = url;
	buffers->userBuf = user;
	buffers->passwdBuf = passwd;
	buffers->issuedBuf = issued;
	buffers->issuerBuf = issuer;
	buffers->btnDel = deleteBtn;
	buffers->btnSave = saveBtn;
	buffers->btnExportCert = exportCertBtn;
	buffers->srvPrefList = serverPrefList;
	buffers->resSel = resSwitch;
	buffers->certSel = digSwitch;
	data->element2 = buffers;

	/*		Connect Signales		*/
	serverSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(serverPrefList));
	gtk_tree_selection_set_mode (serverSel, GTK_SELECTION_SINGLE);
	g_signal_connect(serverSel, "changed", G_CALLBACK(prefServerSelect), data);
	g_signal_connect(G_OBJECT(prefWindow), "destroy", G_CALLBACK(prefExit), buffers);

	g_signal_connect(buffers->btnDel, "clicked", G_CALLBACK(prefServerDelete), data);
	g_signal_connect(buffers->btnSave, "clicked", G_CALLBACK(prefServerSave), data);
	g_signal_connect(buffers->btnExportCert, "clicked", G_CALLBACK(prefExportCert), data);

	g_signal_connect(G_OBJECT(prefWindow), "key_press_event", G_CALLBACK(prefKeyHandler), buffers);

	gtk_container_add(GTK_CONTAINER(prefView), prefList);
	gtk_container_add(GTK_CONTAINER(prefView), prefFrame);
	gtk_container_add(GTK_CONTAINER(prefWindow), prefView);
	gtk_widget_show_all(prefWindow);
}

static void newDialogEntryChanged(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkAssistant			*assistant = GTK_ASSISTANT (data);
	GtkWidget				*box;
	int						curPage;
	GtkEntryBuffer			*buf1, *buf2;
	int						buf1l, buf2l;

	buf1 = buf2 = NULL;
	buf1l = buf2l = 0;
	curPage = gtk_assistant_get_current_page(assistant);
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant),curPage);

	switch(curPage){
		case 1:
			buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "descEntry");
			buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "urlEntry");
			break;
		case 2:
			buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "userEntry");
			buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "passwdEntry");
			break;
		case 3:
			buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "oAuthEntry");
			buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "grantEntry");
			break;
		default:
			break;
	}

	buf1l = gtk_entry_buffer_get_length(buf1);
	buf2l = gtk_entry_buffer_get_length(buf2);
	if(buf1l !=0 && buf2l !=0)
				gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), box, TRUE);
}

static void newDialogClose(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	gtk_widget_destroy(widget);
}

static void newDialogApply(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget					*assistant, *box;
	GtkWidget					*controller;
	GtkEntryBuffer				*buf1, *buf2, *buf3, *buf4;
	sqlite3						*ptr;
	ContactCards_trans_t		*data = trans;

	ptr = data->db;
	assistant = (GtkWidget*)widget;

	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 0);
	controller = (GtkWidget*)g_object_get_data(G_OBJECT(box),"google");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controller))){
		buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "oAuthEntry");
		goto google;
	}

	controller = (GtkWidget*)g_object_get_data(G_OBJECT(box),"fruux");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controller))){
		goto fruux;
	}

	/*	others		*/
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 2);
	buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "userEntry");
	buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "passwdEntry");
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 1);
	buf3 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "descEntry");
	buf4 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "urlEntry");
	newServer(ptr, (char *) gtk_entry_buffer_get_text(buf3), (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), (char *) gtk_entry_buffer_get_text(buf4));
	return;

fruux:
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 2);
	buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "userEntry");
	buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "passwdEntry");
	newServer(ptr, "fruux", (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), "https://dav.fruux.com");
	return;

google:
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 3);
	buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "oAuthEntry");
	buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "grantEntry");
	newServerOAuth(ptr, "google", (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), 1);
	return;
}

static void newDialogConfirm(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkWidget				*box, *hbox;
	GtkWidget				*label;

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Confirm"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);

	gtk_widget_show_all(box);
	gtk_assistant_append_page (GTK_ASSISTANT(widget), box);
	gtk_assistant_set_page_type(GTK_ASSISTANT(widget), box, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(widget), box, _("Confirm"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(widget), box, TRUE);
}

static void newDialogOAuthCredentials(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkWidget				*box, *hbox;
	GtkWidget				*label, *inputoAuth, *inputGrant, *button;
	GtkEntryBuffer			*user, *grant;
	char					*uri = NULL;

	user = gtk_entry_buffer_new(NULL, -1);
	grant = gtk_entry_buffer_new(NULL, -1);

	uri = g_strdup("https://accounts.google.com/o/oauth2/auth?scope=https://www.googleapis.com/auth/carddav&redirect_uri=urn:ietf:wg:oauth:2.0:oob&response_type=code&client_id=741969998490.apps.googleusercontent.com");

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("User"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputoAuth = gtk_entry_new_with_buffer(user);
	g_object_set_data(G_OBJECT(box),"oAuthEntry", user);
	gtk_box_pack_start(GTK_BOX(hbox), inputoAuth, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);


	button = gtk_link_button_new_with_label(uri, _("Request Grant"));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Grant"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputGrant = gtk_entry_new_with_buffer(grant);
	g_object_set_data(G_OBJECT(box),"grantEntry", grant);
	gtk_box_pack_start(GTK_BOX(hbox), inputGrant, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);

	gtk_widget_show_all(box);
	gtk_assistant_append_page (GTK_ASSISTANT(widget), box);
	gtk_assistant_set_page_type(GTK_ASSISTANT(widget), box, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(widget), box, _("OAuth"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(widget), box, FALSE);

	g_signal_connect (G_OBJECT(inputoAuth), "changed", G_CALLBACK(newDialogEntryChanged), widget);
	g_signal_connect (G_OBJECT(inputGrant), "changed", G_CALLBACK(newDialogEntryChanged), widget);
}

static void newDialogUserCredentials(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkWidget				*box, *hbox;
	GtkWidget				*label, *inputUser, *inputPasswd;
	GtkEntryBuffer			*user, *passwd;

	user = gtk_entry_buffer_new(NULL, -1);
	passwd = gtk_entry_buffer_new(NULL, -1);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("User"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputUser = gtk_entry_new_with_buffer(user);
	g_object_set_data(G_OBJECT(box),"userEntry", user);
	gtk_box_pack_start(GTK_BOX(hbox), inputUser, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);


	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Password"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputPasswd = gtk_entry_new_with_buffer(passwd);
	gtk_entry_set_visibility(GTK_ENTRY(inputPasswd), FALSE);
	g_object_set_data(G_OBJECT(box),"passwdEntry", passwd);
	gtk_box_pack_start(GTK_BOX(hbox), inputPasswd, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);

	gtk_widget_show_all(box);
	gtk_assistant_append_page (GTK_ASSISTANT(widget), box);
	gtk_assistant_set_page_type(GTK_ASSISTANT(widget), box, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(widget), box, _("Credentials"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(widget), box, FALSE);

	g_signal_connect (G_OBJECT(inputUser), "changed", G_CALLBACK(newDialogEntryChanged), widget);
	g_signal_connect (G_OBJECT(inputPasswd), "changed", G_CALLBACK(newDialogEntryChanged), widget);
}

static void newDialogServerSettings(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkWidget				*box, *hbox;
	GtkWidget				*label, *inputDesc, *inputUrl;
	GtkEntryBuffer			*desc, *url;

	desc = gtk_entry_buffer_new(NULL, -1);
	url = gtk_entry_buffer_new(NULL, -1);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Description"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputDesc = gtk_entry_new_with_buffer(desc);
	g_object_set_data(G_OBJECT(box),"descEntry", desc);
	gtk_box_pack_start(GTK_BOX(hbox), inputDesc, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);


	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("URL"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputUrl = gtk_entry_new_with_buffer(url);
	g_object_set_data(G_OBJECT(box),"urlEntry", url);
	gtk_box_pack_start(GTK_BOX(hbox), inputUrl, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);

	gtk_widget_show_all(box);
	gtk_assistant_append_page (GTK_ASSISTANT(widget), box);
	gtk_assistant_set_page_type(GTK_ASSISTANT(widget), box, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(widget), box, _("Serversettings"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(widget), box, FALSE);

	g_signal_connect (G_OBJECT(inputDesc), "changed", G_CALLBACK(newDialogEntryChanged), widget);
	g_signal_connect (G_OBJECT(inputUrl), "changed", G_CALLBACK(newDialogEntryChanged), widget);
}

gint newDialogSelectPage(gint curPage, gpointer data){
	printfunc(__func__);

	GtkWidget			*widget;
	GtkWidget			*box;
	GtkWidget			*controller;

	widget = (GtkWidget*)data;
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(widget),curPage);

	if(curPage == 0){
		controller = (GtkWidget*)g_object_get_data(G_OBJECT(box),"google");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controller))){
				return 3;
		}
		controller = (GtkWidget*)g_object_get_data(G_OBJECT(box),"fruux");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controller))){
				return 2;
		}
	}else if (curPage == 2){
		return 4;
	}
	return curPage+1;
}

static void newDialogSelectType(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkWidget				*box;
	GtkWidget				*fruux, *google, *others;

	fruux = gtk_radio_button_new_with_label(NULL, _("fruux"));
	google = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fruux), _("google"));
	others = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fruux), _("others"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(others), TRUE);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
	g_object_set_data(G_OBJECT(box),"google",google);
	g_object_set_data(G_OBJECT(box),"fruux",fruux);
	gtk_box_pack_start (GTK_BOX (box), fruux, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (box), google, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (box), others, TRUE, TRUE, 2);

	gtk_widget_show_all(box);

	gtk_assistant_append_page (GTK_ASSISTANT(widget), box);
	gtk_assistant_set_page_type(GTK_ASSISTANT(widget), box, GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_title(GTK_ASSISTANT(widget), box, _("Select Server"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(widget), box, TRUE);
}

static void newDialog(GtkWidget *do_widget, gpointer trans){
	printfunc(__func__);

	GtkWidget 			*assistant = NULL;

	assistant = gtk_assistant_new ();
	gtk_window_set_destroy_with_parent(GTK_WINDOW(assistant), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (assistant), -1, 300);

	newDialogSelectType(assistant, trans);
	newDialogServerSettings(assistant, trans);
	newDialogUserCredentials(assistant, trans);
	newDialogOAuthCredentials(assistant, trans);
	newDialogConfirm(assistant, trans);

	gtk_widget_show_all(assistant);
	gtk_assistant_set_forward_page_func(GTK_ASSISTANT(assistant), newDialogSelectPage, assistant, NULL);

	/*		Connect Signales		*/
	g_signal_connect(G_OBJECT(assistant), "close", G_CALLBACK(newDialogClose), NULL);
	g_signal_connect(G_OBJECT(assistant), "cancel", G_CALLBACK(newDialogClose), NULL);
	g_signal_connect(G_OBJECT(assistant), "apply", G_CALLBACK(newDialogApply), trans);
}

void guiInit(sqlite3 *ptr){
	printfunc(__func__);

	GtkWidget			*mainVBox, *mainToolbar, *mainStatusbar, *mainContent;
	GtkWidget			*addressbookWindow;
	GtkWidget			*contactBox, *contactWindow, *contactView, *scroll;
	GtkWidget			*serverCombo;
	GtkToolItem			*comboItem, *prefItem, *aboutItem, *sep, *newServer, *syncItem, *exportItem;
	GtkTreeSelection	*bookSel, *contactSel;
	GtkTextBuffer		*dataBuffer;
	GSList 				*cleanUpList = g_slist_alloc();
	ContactCards_trans_t		*transBook = NULL;
	ContactCards_trans_t		*transContact = NULL;
	ContactCards_trans_t		*transPref = NULL;
	ContactCards_trans_t		*transNew = NULL;
	ContactCards_trans_t		*transSync = NULL;

	gtk_init(NULL, NULL);

	mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(mainWindow), "ContactCards");
	gtk_window_set_default_size(GTK_WINDOW(mainWindow), 760,496);

	mainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	/*		Toolbar					*/
	mainToolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(mainToolbar), GTK_TOOLBAR_ICONS);

	newServer = gtk_tool_button_new(NULL, _("New"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(newServer), _("New"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (newServer), "document-new");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), newServer, -1);

	prefItem = gtk_tool_button_new(NULL, _("Preferences"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(prefItem), _("Preferences"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (prefItem), "preferences-system");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), prefItem, -1);

	exportItem = gtk_tool_button_new(NULL, _("Export"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(exportItem), _("Export"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (exportItem), "document-save");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), exportItem, -1);

	syncItem = gtk_tool_button_new(NULL, _("Refresh"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(syncItem), _("Refresh"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (syncItem), "view-refresh");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), syncItem, -1);

	serverCombo = comboInit(ptr);
	comboItem = gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(comboItem), serverCombo);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), comboItem, -1);

	sep = gtk_separator_tool_item_new();
	gtk_tool_item_set_expand(sep, TRUE);
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(sep), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), sep, -1);
	aboutItem = gtk_tool_button_new(NULL, _("About"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(aboutItem), _("About"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (aboutItem), "help-about");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), aboutItem, -1);

	/*		Statusbar				*/
	mainStatusbar = gtk_statusbar_new();

	/*		mainContent				*/
	mainContent = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	/*		Addressbookstuff		*/
	addressbookWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(addressbookWindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(addressbookWindow, 128, -1);
	addressbookList = gtk_tree_view_new();
	listInit(addressbookList);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(addressbookList), FALSE);

	/*		Contactstuff			*/
	contactBox = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	contactWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(contactWindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(contactWindow, 192, -1);
	dataBuffer = gtk_text_buffer_new(NULL);
	contactView = gtk_text_view_new_with_buffer(dataBuffer);
	gtk_container_set_border_width(GTK_CONTAINER(contactView), 10);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(scroll), contactView);
	contactList = gtk_tree_view_new();
	listInit(contactList);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(contactList), FALSE);

	/*		preference dialog		*/
	transPref = g_new(ContactCards_trans_t, 1);
	transPref->db = ptr;
	cleanUpList = g_slist_append(cleanUpList, transPref);

	/*		new Server dialog		*/
	transNew = g_new(ContactCards_trans_t, 1);
	transNew->db = ptr;
	cleanUpList = g_slist_append(cleanUpList, transNew);

	/*		sync Server 			*/
	transSync = g_new(ContactCards_trans_t, 1);
	transSync->db = ptr;
	transSync->element = mainStatusbar;
	cleanUpList = g_slist_append(cleanUpList, transSync);

	/*		Connect Signales		*/
	transBook = g_new(ContactCards_trans_t, 1);
	transBook->db = ptr;
	cleanUpList = g_slist_append(cleanUpList, transBook);
	bookSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(addressbookList));
	gtk_tree_selection_set_mode (bookSel, GTK_SELECTION_SINGLE);
	g_signal_connect(bookSel, "changed", G_CALLBACK(selBook), transBook);

	transContact = g_new(ContactCards_trans_t, 1);
	transContact->db = ptr;
	transContact->element = contactView;
	cleanUpList = g_slist_append(cleanUpList, transContact);
	contactSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(contactList));
	gtk_tree_selection_set_mode (contactSel, GTK_SELECTION_SINGLE);
	g_signal_connect(contactSel, "changed", G_CALLBACK(selContact), transContact);

	g_signal_connect(G_OBJECT(mainWindow), "key_press_event", G_CALLBACK(guiKeyHandler), cleanUpList);
	g_signal_connect(G_OBJECT(mainWindow), "destroy", G_CALLBACK(guiExit), cleanUpList);
	g_signal_connect(G_OBJECT(prefItem), "clicked", G_CALLBACK(prefWindow), transPref);
	g_signal_connect(G_OBJECT(aboutItem), "clicked", G_CALLBACK(dialogAbout), NULL);
	g_signal_connect(G_OBJECT(newServer), "clicked", G_CALLBACK(newDialog), transNew);
	g_signal_connect(G_OBJECT(syncItem), "clicked", G_CALLBACK(syncServer), transSync);
	g_signal_connect(G_OBJECT(exportItem), "clicked", G_CALLBACK(dialogExportContacts), transPref);

	/*		Put it all together		*/
	gtk_box_pack_start(GTK_BOX(mainVBox), mainToolbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(addressbookWindow), addressbookList);
	gtk_container_add(GTK_CONTAINER(mainContent), addressbookWindow);
	gtk_container_add(GTK_CONTAINER(contactWindow), contactList);
	gtk_container_add(GTK_CONTAINER(contactBox), contactWindow);
	gtk_container_add(GTK_CONTAINER(contactBox), scroll);
	gtk_container_add(GTK_CONTAINER(mainContent), contactBox);
	gtk_box_pack_start(GTK_BOX(mainVBox), mainContent, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainVBox), mainStatusbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(mainWindow), mainVBox);
	gtk_widget_show_all(mainWindow);
}
