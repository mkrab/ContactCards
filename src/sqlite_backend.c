/* Copyright (c) 2013-2014 Florian L. <dev@der-flo.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "contactcards.h"

/**
 * doSimpleRequest - sends a simple request to the local database
 *	@ptr			pointer to the database
 *	@sql_query		query to send to the database
 *	@func			functionname which the request come from
 *
 *	This function sends a request to the database and handles possible
 *	errors
 */
void doSimpleRequest(sqlite3 *ptr, char *sql_query, const char *func){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	int					ret;

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		verboseCC("[%s] %s caused %d - %s\n", __func__, func,  sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		sqlite3_mutex_leave(dbMutex);
		return;
	}

	debugCC("[%s] %s\n", __func__, sql_query);

	ret = sqlite3_step(vm);

	if (ret != SQLITE_DONE){
		verboseCC("[%s] %s caused %d - %s\n", __func__, func,  sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		sqlite3_mutex_leave(dbMutex);
		return;
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
	sqlite3_mutex_leave(dbMutex);
}

/**
 * dbCleanUp - remove stuff, which are marked as temporary
 */
static void dbCleanUp(sqlite3 *ptr){
	printfunc(__func__);

	GSList		*temporary;

	temporary = getListInt(ptr, "contacts", "contactID", 91, "flags", CONTACTCARDS_TMP, "", "", "", "");

	while(temporary){
		GSList				*next = temporary->next;
		int					id = GPOINTER_TO_INT(temporary->data);
		if(id == 0){
			temporary = next;
			continue;
		}
		dbRemoveItem(ptr, "contacts", 2, "", "", "contactID", id);
				temporary = next;
	}
	g_slist_free(temporary);

}

/**
 * dbClose - close the connection to the local database
 */
void dbClose(sqlite3 *ptr){
	printfunc(__func__);

	dbCleanUp(ptr);

	sqlite3_close(ptr);
}

/**
 * dbCreate - create a new local database
 */
void dbCreate(sqlite3 *ptr){
	printfunc(__func__);

	char				**errmsg = NULL;
	int 				ret;

	ret = sqlite3_exec(ptr, "CREATE TABLE IF NOT EXISTS contacts( \
	contactID INTEGER PRIMARY KEY AUTOINCREMENT,\
	addressbookID INTEGER, \
	flags INTEGER default 0, \
	etag TEXT, \
	href TEXT, \
	vCard TEXT, \
	displayname TEXT);", NULL, NULL, errmsg);

	if (ret != SQLITE_OK)	goto sqlError;

	ret = sqlite3_exec(ptr, "CREATE TABLE IF NOT EXISTS oAuthServer( \
	oAuthID INTEGER PRIMARY KEY AUTOINCREMENT,\
	flags INTEGER default 0, \
	desc TEXT, \
	clientID TEXT, \
	clientSecret TEXT, \
	davURI TEXT, \
	scope TEXT, \
	grantURI TEXT, \
	tokenURI TEXT, \
	responseType TEXT, \
	redirURI TEXT, \
	grantType TEXT);", NULL, NULL, errmsg);

	if (ret != SQLITE_OK)	goto sqlError;

	ret = sqlite3_exec(ptr, "CREATE TABLE IF NOT EXISTS cardServer( \
	serverID INTEGER PRIMARY KEY AUTOINCREMENT,\
	flags INTEGER default 0, \
	desc TEXT,\
	user TEXT, \
	passwd TEXT, \
	srvUrl TEXT,\
	authority TEXT,\
	resources INTEGER default 1,\
	isOAuth INTEGER default 0,\
	oAuthType INTEGER,\
	oAuthAccessGrant TEXT,\
	oAuthAccessToken TEXT,\
	oAuthRefreshToken TEXT);", NULL, NULL, errmsg);

	if (ret != SQLITE_OK)	goto sqlError;

	ret = sqlite3_exec(ptr, "CREATE TABLE IF NOT EXISTS addressbooks( \
	addressbookID INTEGER PRIMARY KEY AUTOINCREMENT,\
	flags INTEGER default 0, \
	cardServer INTEGER, \
	displayname TEXT, \
	path TEXT, \
	postURI TEXT, \
	syncToken TEXT, \
	syncMethod INTEGER);", NULL, NULL, errmsg);

	if (ret != SQLITE_OK)	goto sqlError;

	ret = sqlite3_exec(ptr, "CREATE TABLE IF NOT EXISTS certs( \
	certID INTEGER PRIMARY KEY AUTOINCREMENT,\
	serverID INTEGER, \
	trustFlag INTEGER default 2,\
	digest TEXT, \
	issued TEXT, \
	issuer TEXT, \
	cert TEXT);", NULL, NULL, errmsg);

	if (ret != SQLITE_OK)	goto sqlError;

	return;

sqlError:
	verboseCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
	return;
}

/**
 * countElements - returns quantity of a element in a table of the local database
 * returns 0 if it does not exist
 */

int countElements(sqlite3 *ptr, char *tableName, int rows, char *row1, int value1, char *row2, char *value2, char *row3, char *value3){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int					count = 0;
	int 				ret;

	switch(rows){
		case 1:
			sql_query = sqlite3_mprintf("SELECT COUNT(*) FROM %q WHERE %q = '%d';", tableName, row1, value1);
			break;
		case 2:
			sql_query = sqlite3_mprintf("SELECT COUNT(*) FROM %q WHERE %q = '%q';", tableName, row2, value2);
			break;
		case 12:
			sql_query = sqlite3_mprintf("SELECT COUNT(*) FROM %q WHERE %q = '%d' AND %q = '%q';", tableName, row1, value1, row2, value2);
			break;
		case 23:
			sql_query = sqlite3_mprintf("SELECT COUNT(*) FROM %q WHERE %q = '%q' AND %q = '%q';", tableName, row2, value2,  row3, value3);
			break;
		case 123:
			sql_query = sqlite3_mprintf("SELECT COUNT(*) FROM %q WHERE %q = '%d' AND %q = '%q' AND %q = '%q';", tableName, row1, value1, row2, value2, row3, value3);
			break;
		default:
			verboseCC("[%s] can't handle this number: %d\n", __func__, rows);
			return 0;
	}

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		verboseCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		sqlite3_mutex_leave(dbMutex);
		return 0;
	}

	while(sqlite3_step(vm) != SQLITE_DONE){
			count = sqlite3_column_int(vm, 0);
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
	sqlite3_mutex_leave(dbMutex);

	return count;
}

/**
 * setSingleInt - sets an integer value to a row in a table
 */
void setSingleInt(sqlite3 *ptr, char *tableName, char *setValue, int newValue, char *row1, int value1){
	printfunc(__func__);

	char 				*sql_query;

	sql_query = sqlite3_mprintf("UPDATE %q SET %q = '%d' WHERE %q = '%d';", tableName, setValue, newValue, row1, value1);

	doSimpleRequest(ptr, sql_query, __func__);
}

/**
 * setSingleChar - sets a char value to a row in a table
 */
void setSingleChar(sqlite3 *ptr, char *tableName, char *setValue, char *newValue, char *row1, int value1){
	printfunc(__func__);

	char 				*sql_query;

	sql_query = sqlite3_mprintf("UPDATE %q SET %q = %Q WHERE %q = '%d';", tableName, setValue, newValue, row1, value1);

	doSimpleRequest(ptr, sql_query, __func__);
}

/**
 * getListInt - returns a list of integers of a table of the local database
 */
GSList *getListInt(sqlite3 *ptr, char *tableName, char *selValue, int selRow, char *row1, int value1, char *row2, char *value2, char *row3, char *value3){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int 				ret;
	GSList				*list = g_slist_alloc();

	g_strstrip(tableName);
	g_strstrip(selValue);
	g_strstrip(row1);
	g_strstrip(row2);
	g_strstrip(value2);
	g_strstrip(row3);
	g_strstrip(value3);

	switch(selRow){
		case 1:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d';", selValue, tableName, row1, value1);
			break;
		case 12:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q';", selValue, tableName, row1, value1, row2, value2);
			break;
		case 2:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%q';", selValue, tableName, row2, value2);
			break;
		case 0:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q;", selValue, tableName);
			break;
		case 23:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%q' AND %q = '%q';", selValue, tableName, row2, value2,  row3, value3);
			break;
		case 123:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q' AND %q = '%q';", selValue, tableName, row1, value1, row2, value2, row3, value3);
			break;
		case 91:	/* SQL Query using bitwise operation	*/
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE (%q & %d) = %d;", selValue, tableName, row1, value1, value1);
			break;
		default:
			verboseCC("[%s] can't handle this number: %d\n", __func__, selRow);
			return NULL;
	}

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		verboseCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		sqlite3_mutex_leave(dbMutex);
		return list;
	}

	debugCC("[%s] %s\n", __func__, sql_query);

	while(sqlite3_step(vm) != SQLITE_DONE){
		list = g_slist_append(list,  GINT_TO_POINTER(sqlite3_column_int(vm, 0)));
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
	sqlite3_mutex_leave(dbMutex);

	return list;

}

/**
 * getSingleInt - returns a single integer value
 *	@ptr			pointer to the database
 *	@tableName		name of the table in the database
 *	@selValue		name of the column which will be selected
 *	@selRow			selected combination of rowX and/or rowY
 *	@row1			row 1 to select
 *	@value1			value for row 1
 *	@row2			row 2 to select
 *	@value2			value for row 2
 *	@row3			row 3 to select
 *	@value3			value for row 3
 *
 *	This functions returns a single int if there is no error on the
 *	request to the database
 */
int getSingleInt(sqlite3 *ptr, char *tableName, char *selValue, int selRow, char *row1, int value1, char *row2, char *value2, char *row3, char *value3){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int					ret;
	int					count = 0;
	int					retValue = 0;

	g_strstrip(tableName);
	g_strstrip(selValue);
	g_strstrip(row1);
	g_strstrip(row2);
	g_strstrip(value2);
	g_strstrip(row3);
	g_strstrip(value3);

	switch(selRow){
		case 1:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d';", selValue, tableName, row1, value1);
			break;
		case 12:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q';", selValue, tableName, row1, value1, row2, value2);
			break;
		case 2:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%q';", selValue, tableName, row2, value2);
			break;
		case 23:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%q' AND %q = '%q';", selValue, tableName, row2, value2, row3, value3);
			break;
		case 123:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q' AND %q = '%q';", selValue, tableName, row1, value1, row2, value2, row3, value3);
			break;
		default:
			verboseCC("[%s] can't handle this number: %d\n", __func__, selRow);
			return -1;
	}

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		verboseCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		sqlite3_mutex_leave(dbMutex);
		return count;
	}

	debugCC("[%s] %s\n", __func__, sql_query);

	while(sqlite3_step(vm) != SQLITE_DONE) {
		if(sqlite3_column_text(vm, 0) == NULL){
			retValue = 0;
		} else {
			retValue = sqlite3_column_int(vm, 0);
		}
		count++;
	}

	if(count != 1){
		verboseCC("[%s] there is more than one returning value. can't handle %d values\n", __func__, count);
		sqlite3_mutex_leave(dbMutex);
		return -1;
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
	sqlite3_mutex_leave(dbMutex);

	return retValue;
}

/**
 * getSingleChar - returns a single char value
 *	@ptr			pointer to the database
 *	@tableName		name of the table in the database
 *	@selValue		name of the column which will be selected
 *	@selRow			selected combination of rowX and/or rowY
 *	@row1			row 1 to select
 *	@value1			value for row 1
 *	@row2			row 2 to select
 *	@value2			value for row 2
 *	@row3			row 3 to select
 *	@value3			value for row 3
 *	@row4			row 4 to select
 *	@value4			value for row 4
 *
 *	This functions returns a single char if there is no error on the
 *	request to the database
 */
char *getSingleChar(sqlite3 *ptr, char *tableName, char *selValue, int selRow, char *row1, int value1, char *row2, char *value2, char *row3, char *value3, char *row4, int value4){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int					ret;
	int					count = 0;
	char				*retValue = NULL;

	g_strstrip(tableName);
	g_strstrip(selValue);
	g_strstrip(row1);
	g_strstrip(row2);
	g_strstrip(value2);
	g_strstrip(row3);
	g_strstrip(value3);
	g_strstrip(row4);

	switch(selRow){
		case 1:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d';", selValue, tableName, row1, value1);
			break;
		case 12:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q';", selValue, tableName, row1, value1, row2, value2);
			break;
		case 23:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%q' AND %q = '%q';", selValue, tableName, row2, value2, row3, value3);
			break;
		case 14:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%d';", selValue, tableName, row1, value1, row4, value4);
			break;
		case 123:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q' AND %q = '%q';", selValue, tableName, row1, value1, row2, value2, row3, value3);
			break;
		default:
			verboseCC("[%s] can't handle this number: %d\n", __func__, selRow);
			return NULL;
	}

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		verboseCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		sqlite3_mutex_leave(dbMutex);
		return NULL;
	}

	debugCC("[%s] %s\n", __func__, sql_query);

	while(sqlite3_step(vm) != SQLITE_DONE) {
		if(sqlite3_column_text(vm, 0) == NULL){
			retValue = g_strndup(" ", 1);
		} else {
			retValue = g_strndup(sqlite3_column_text(vm, 0), strlen(sqlite3_column_text(vm, 0)));
		}
		count++;
	}

	if(count != 1){
		verboseCC("[%s] there is more than one returning value. can't handle %d values'\n", __func__, count);
		sqlite3_mutex_leave(dbMutex);
		return NULL;
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
	sqlite3_mutex_leave(dbMutex);

	return retValue;
}

/**
 * newOAuthEntity - insert a new OAuth entity into the local database
 */
void newOAuthEntity(sqlite3 *ptr, char *desc, char *clientID, char *clientSecret, char *davURI, char *scope, char *grantURI, char *tokenURI, char *responseType, char *redirURI, char *grantType){
	printfunc(__func__);

	char		 		*sql_query;

	if(countElements(ptr, "oAuthServer", 2, "", 0, "desc", desc, "", "") !=0){
				return;
	}

	sql_query = sqlite3_mprintf("INSERT INTO oAuthServer (desc, clientID, clientSecret, davURI, scope , grantURI, tokenURI, responseType, redirURI, grantType) VALUES ('%q', '%q','%q', '%q','%q', '%q', '%q', '%q', '%q', '%q');", desc, clientID, clientSecret, davURI, scope, grantURI, tokenURI, responseType, redirURI, grantType);

	doSimpleRequest(ptr, sql_query, __func__);
}

/**
 * newServerOAuth - creates a new server using OAuth
 */
void newServerOAuth(sqlite3 *ptr, char *desc, char *newuser, char *newGrant, int oAuthEntity){
	printfunc(__func__);

	char		 		*sql_query = NULL;
	char				*davBase = NULL;
	GSList				*retList;
	int					serverID;

	g_strstrip(desc);
	g_strstrip(newuser);
	g_strstrip(newGrant);

	davBase = getSingleChar(ptr, "oAuthServer", "davURI", 1, "oAuthID", oAuthEntity, "", "", "", "", "", 0);

	retList = getListInt(ptr, "cardServer", "serverID", 12, "oAuthType", oAuthEntity, "user", newuser, "", "");

	if(g_slist_length(retList) != 1) {
		verboseCC("[%s] there is something similar\n", __func__);
		g_slist_free(retList);
		return;
	}
	g_slist_free(retList);

	sql_query = sqlite3_mprintf("INSERT INTO cardServer (desc, user, srvUrl, isOAuth, oAuthType) VALUES ('%q','%q','%q','%d','%d');", desc, newuser,  davBase, 1, oAuthEntity);

	doSimpleRequest(ptr, sql_query, __func__);

	serverID = getSingleInt(ptr, "cardServer", "serverID", 12, "oAuthType", oAuthEntity, "user", newuser, "", "");
	setSingleChar(ptr, "cardServer", "oAuthAccessGrant", newGrant, "serverID", serverID);

	g_mutex_lock(&mutex);
	serverConnectionTest(serverID);

	oAuthAccess(ptr, serverID, oAuthEntity, DAV_REQ_GET_TOKEN);
	g_mutex_unlock(&mutex);

	g_free(davBase);

	return;
}

/**
 * newServer - create a new server
 */
void newServer(sqlite3 *ptr, char *desc, char *user, char *passwd, char *url){
	printfunc(__func__);

	ne_uri				uri;
	char		 		*sql_query;
	GSList				*retList;
	int					serverID;

	g_strstrip(desc);
	g_strstrip(user);
	g_strstrip(passwd);
	g_strstrip(url);

	ne_uri_parse(url, &uri);

	if(uri.host == NULL){
		verboseCC("[%s] ne_uri_parse didn't find host in %s", __func__, url);
		return;
	}
	retList = getListInt(ptr, "cardServer", "serverID", 23, "", 0, "user", user, "authority", uri.host);

	if(g_slist_length(retList) != 1){
		while(retList){
			GSList *next = retList->next;
			verboseCC("[%s] checking: %d\n", __func__, GPOINTER_TO_INT(retList->data));
			if(countElements(ptr, "addressbooks", 12, "cardServer", GPOINTER_TO_INT(retList->data), "path", uri.path, "", "") !=0){
				verboseCC("[%s] there is something similar\n", __func__);
				g_slist_free(retList);
				return;
			}
			if(countElements(ptr, "cardServer", 23, "", 0, "user", user, "srvUrl", url) !=0){
				verboseCC("[%s] there is something similar\n", __func__);
				g_slist_free(retList);
				return;
			}
			if(countElements(ptr, "cardServer", 23, "", 0, "user", user, "authority", uri.host) !=0){
				verboseCC("[%s] there is something similar\n", __func__);
				g_slist_free(retList);
				return;
			}
			retList = next;
		}
	}
	g_slist_free(retList);

	sql_query = sqlite3_mprintf("INSERT INTO cardServer (desc, user, passwd, srvUrl, authority) VALUES ('%q','%q','%q','%q', '%q');", desc, user, passwd, url, uri.host);

	doSimpleRequest(ptr, sql_query, __func__);

	serverID = getSingleInt(ptr, "cardServer", "serverID", 23, "", 0, "user", user, "srvUrl", url);
	g_mutex_lock(&mutex);
	serverConnectionTest(serverID);
	g_mutex_unlock(&mutex);
	ne_uri_free(&uri);

}

/**
 * showServer - list all server of a local database 
 * 
 * this function is for debugging only, so far
 */
void showServer(sqlite3 *ptr){
	printfunc(__func__);

	sqlite3_stmt		*vm;

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	sqlite3_prepare_v2(ptr, "SELECT * FROM cardServer", -1, &vm, NULL);
	while(sqlite3_step(vm) != SQLITE_DONE){
		printf("[%i]\t%s\t%s\n", sqlite3_column_int(vm, 0), sqlite3_column_text(vm, 1), sqlite3_column_text(vm, 4));
	}

	sqlite3_finalize(vm);
	sqlite3_mutex_leave(dbMutex);
}

/**
 * showAddressbooks - list all address books of a local database 
 * 
 * this function is for debugging only, so far
 */
void showAddressbooks(sqlite3 *ptr){
	printfunc(__func__);

	sqlite3_stmt		*vm;

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	sqlite3_prepare_v2(ptr, "SELECT * FROM addressbooks", -1, &vm, NULL);
	while(sqlite3_step(vm) != SQLITE_DONE){
		printf("[%i - %i]\t%s\t%s\t%s\n", sqlite3_column_int(vm, 0), sqlite3_column_int(vm, 1), sqlite3_column_text(vm, 2), sqlite3_column_text(vm, 3), sqlite3_column_text(vm, 4));
	}

	sqlite3_finalize(vm);
	sqlite3_mutex_leave(dbMutex);
}

/**
 * cleanUpRequest - generic function to remove stuff from the local database
 */
void cleanUpRequest(sqlite3 *ptr, int id, int type){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int					ret;

	switch(type){
		// addressbooks
		case 0:
				sql_query = sqlite3_mprintf("SELECT addressbookID FROM addressbooks WHERE cardServer = '%d';", id);
				break;
		// contacts
		case 1:
				sql_query = sqlite3_mprintf("SELECT contactID FROM contacts WHERE addressbookID = '%d';", id);
				break;
	}

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		verboseCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		sqlite3_mutex_leave(dbMutex);
		return;
	}

	debugCC("[%s] %s\n", __func__, sql_query);

	while(sqlite3_step(vm) != SQLITE_DONE) {
		switch(type){
			// addressbooks
			case 0:
					cleanUpRequest(ptr, sqlite3_column_int(vm, 0), 1);
					dbRemoveItem(ptr, "addressbooks", 2, "", "", "addressbookID", sqlite3_column_int(vm, 0));
					break;
			// contacts
			case 1:
					dbRemoveItem(ptr, "contacts", 2, "", "", "contactID", sqlite3_column_int(vm, 0));
					break;
		}
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
	sqlite3_mutex_leave(dbMutex);
}

/**
 * dbRemoveItem - remove a single item from the local database
 */
void dbRemoveItem(sqlite3 *ptr, char *tableName, int selRow, char *row1, char *value1, char *row2, int value2){
	printfunc(__func__);

	char 				*sql_query;

	switch(selRow){
		case 1:
			sql_query = sqlite3_mprintf("DELETE FROM %q WHERE %q = '%q';", tableName, row1, value1);
			break;
		case 2:
			sql_query = sqlite3_mprintf("DELETE FROM %q WHERE %q = '%d';", tableName, row2, value2);
			break;
		case 12:
			sql_query = sqlite3_mprintf("DELETE FROM %q WHERE %q = '%q' AND %q = '%d';", tableName, row1, value1, row2, value2);
			break;
		default:
			verboseCC("[%s] can't handle this number: %d\n", __func__, selRow);
			return;
	}

	doSimpleRequest(ptr, sql_query, __func__);
}

/**
 * readCardServerCredits - returns the credentials from a local database
 */
void readCardServerCredits(int serverID, credits_t *key, sqlite3 *ptr){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query;
	int					ret;

	sql_query = sqlite3_mprintf("SELECT user, passwd FROM cardServer WHERE serverID = '%d';", serverID);

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		verboseCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		sqlite3_mutex_leave(dbMutex);
		return;
	}

	debugCC("[%s] %s\n", __func__, sql_query);

	while(sqlite3_step(vm) != SQLITE_DONE) {
		key->user	= g_strdup((char *) sqlite3_column_text(vm, 0));
		key->passwd	= g_strdup((char *) sqlite3_column_text(vm, 1));
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
	sqlite3_mutex_leave(dbMutex);
}

/**
 * fixURI - fix a URI
 *
 * This is a simple try to fix the URI of a vCard
 * It is expected that only the basepath is broken
 */
static char *fixURI(char *base,char *corrupted){
	printfunc(__func__);

	char			*new = NULL;
	char			**elements = g_strsplit(corrupted, "/", 0);
	char			*tmp;
	int				i = 0;

	do{
		i++;
	} while(elements[i] != NULL);

	tmp = g_strdup(elements[i-1]);

	new = g_strconcat(base, tmp, NULL);

	g_free(tmp);
	g_strfreev(elements);

	return new;
}

/**
 * contactHandle - update a new vCard at the local database
 */
void contactHandle(sqlite3 *ptr, char *href, char *etag, int serverID, int addressbookID, ne_session *sess){
	printfunc(__func__);

	char 				*sql_query;
	char				*basePath;
	int					contactID;

	basePath = getSingleChar(ptr, "addressbooks", "path", 1, "addressbookID", addressbookID, "", "", "", "", "", 0);
	if(strlen(basePath) == 1) return;
	/*	This is for modifying the href	*/
	if(strncmp(href, basePath, strlen(basePath))){
		char				*tmp = NULL;
		tmp = fixURI(basePath, href);
		if(strncmp(tmp, basePath, strlen(basePath))){
			return;
		} else {
			href = tmp;
		}
	}

	if(strlen(href) == strlen(basePath)){
		/* If the href == basePath, then go on	*/
		return;
	}
	g_free(basePath);

	if(countElements(ptr, "contacts", 123, "addressbookID", addressbookID, "etag", etag, "href", href) != 0){
		return;
	}

	if(countElements(ptr, "contacts", 12, "addressbookID", addressbookID, "href", href, "", "") != 0){
		sql_query = sqlite3_mprintf("UPDATE contacts SET etag = %Q WHERE addressbookID = %d AND href = %Q;", etag, addressbookID, href);
	} else {
		sql_query = sqlite3_mprintf("INSERT INTO contacts (addressbookID, etag, href) VALUES ('%d', %Q, %Q);", addressbookID, etag, href);
	}

	doSimpleRequest(ptr, sql_query, __func__);

	contactID = getSingleInt(ptr, "contacts", "contactID", 12, "addressbookID", addressbookID, "href", href, "", "");

	if(contactID == -1) return;

	serverRequest(DAV_REQ_GET, serverID, contactID, sess, ptr);

}

/**
 * newContact - create a new vCard at the local database
 */
int newContact(sqlite3 *ptr, int addressbookID, char *card){
	printfunc(__func__);

	char		 		*sql_query;
	char				*basePath;
	char				*cardPath;
	char				*path;
	int					newID;

	basePath = getSingleChar(ptr, "addressbooks", "path", 1, "addressbookID", addressbookID, "", "", "", "", "", 0);
	cardPath = getSingleCardAttribut(CARDTYPE_UID, card);

	path = g_strconcat(g_strstrip(basePath), g_strstrip(cardPath), ".vcf", NULL);

	sql_query = sqlite3_mprintf("INSERT INTO contacts (addressbookID, vCard, href, flags) VALUES ('%d','%q', '%q', '%d');", addressbookID, card, path, CONTACTCARDS_TMP);

	doSimpleRequest(ptr, sql_query, __func__);

	g_free(basePath);
	g_free(cardPath);
	g_free(path);

	newID = sqlite3_last_insert_rowid(ptr);

	setDisplayname(ptr, newID, card);

	return newID;
}

/**
 * newAddressbook - create a new address book at the local database
 */
void newAddressbook(sqlite3 *ptr, int cardServer, char *displayname, char *path){
	printfunc(__func__);

	char		 		*sql_query;

	if(countElements(ptr, "addressbooks", 123, "cardServer", cardServer, "displayname", displayname, "path", path) != 0){
		return;
	}

	sql_query = sqlite3_mprintf("INSERT INTO addressbooks (cardServer, displayname, path) VALUES ('%d','%q','%q');", cardServer, displayname, path);

	doSimpleRequest(ptr, sql_query, __func__);

}

/**
 * showContacts - list all vCards of a local database 
 * 
 * this function is for debugging only, so far
 */
void showContacts(sqlite3 *ptr){
	printfunc(__func__);

	sqlite3_stmt		*vm;

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	sqlite3_prepare_v2(ptr, "SELECT * FROM contacts", -1, &vm, NULL);

	while(sqlite3_step(vm) != SQLITE_DONE)
	{
		printf("cID:%i\taID: %d\t%s\t%s\n", sqlite3_column_int(vm, 0), sqlite3_column_int(vm, 1), sqlite3_column_text(vm, 3), sqlite3_column_text(vm, 2));
	}

	sqlite3_finalize(vm);
	sqlite3_mutex_leave(dbMutex);
}

/**
 * setDisplayname - set the displaying name of a vCard
 */
void setDisplayname(sqlite3 *ptr, int contactID, char *vData){
	printfunc(__func__);

	char			*displayName = NULL;
	char	 		*sql_query = NULL;

	displayName = getSingleCardAttribut(CARDTYPE_FN, vData);

	if(strlen(g_strstrip(displayName)) == 0){
		sql_query = sqlite3_mprintf("UPDATE contacts SET displayname = '(no name)' WHERE contactID = '%d';", contactID);
	} else {
		sql_query = sqlite3_mprintf("UPDATE contacts SET displayname = %Q WHERE contactID = '%d';", g_strstrip(displayName), contactID);
	}
	doSimpleRequest(ptr, sql_query, __func__);
	g_free(displayName);
}

/**
 * updateAddressbooks - update the flags for the address books
 */
void updateAddressbooks(sqlite3 *ptr, GSList *list){
	printfunc(__func__);

	ContactCards_aBooks_t		*item;
	GSList						*next;

	while(list){
		int							check = 0;
		int							flags = 0;
		char				 		*sql_query = NULL;

		next = list->next;
		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_aBooks_t *)list->data;
		flags = getSingleInt(ptr, "addressbooks", "syncMethod", 1, "addressbookID", item->aBookID, "", "", "", "");
		if(flags == -1)
			goto stepForward;
		check = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->check));
		if(check){
			/* Clear the bit	*/
			flags &= ~(1<<DAV_ADDRBOOK_DONT_SYNC);
		} else {
			/* Set the bit		*/
			flags |= (1<<DAV_ADDRBOOK_DONT_SYNC);
		}
		verboseCC("Updating %d\n", item->aBookID);

		sql_query = sqlite3_mprintf("UPDATE addressbooks SET syncMethod = '%d' WHERE addressbookID = '%d';", flags, item->aBookID);
		doSimpleRequest(ptr, sql_query, __func__);
stepForward:
		list = next;
	}
}

/**
 * updateServerDetails - update changes to the server settings
 */
void updateServerDetails(sqlite3 *ptr, int srvID, const gchar *newDesc, const gchar *newUrl, const gchar *newUser, const gchar *newPw, gboolean certSel){
	printfunc(__func__);

	char				*oldDesc = NULL, *oldUrl = NULL, *oldUser = NULL, *oldPw = NULL;

	oldDesc = getSingleChar(ptr, "cardServer", "desc", 1, "serverID", srvID, "", "", "", "", "", 0);
	oldUrl = getSingleChar(ptr, "cardServer", "srvUrl", 1, "serverID", srvID, "", "", "", "", "", 0);
	oldUser = getSingleChar(ptr, "cardServer", "user", 1, "serverID", srvID, "", "", "", "", "", 0);
	oldPw =  getSingleChar(ptr, "cardServer", "passwd", 1, "serverID", srvID, "", "", "", "", "", 0);

	if(g_strcmp0(oldDesc, newDesc))
		setSingleChar(ptr, "cardServer", "desc", (char *) newDesc, "serverID", srvID);
	if(g_strcmp0(oldUrl, newUrl))
		setSingleChar(ptr, "cardServer", "srvUrl", (char *) newUrl, "serverID", srvID);
	if(g_strcmp0(oldUser, newUser))
		setSingleChar(ptr, "cardServer", "user", (char *) newUser, "serverID", srvID);
	if(g_strcmp0(oldPw, newPw))
		setSingleChar(ptr, "cardServer", "passwd", (char *) newPw, "serverID", srvID);
	if(certSel == TRUE)
		setSingleInt(ptr, "certs", "trustFlag", (int) ContactCards_DIGEST_TRUSTED, "serverID", srvID);
	if(certSel == FALSE)
		setSingleInt(ptr, "certs", "trustFlag", (int) ContactCards_DIGEST_UNTRUSTED, "serverID", srvID);

	g_free(oldDesc);
	g_free(oldUrl);
	g_free(oldUser);
	g_free(oldPw);
}

/**
 * updatePostURI - update the URI the new vCards are send to
 */
void updatePostURI(sqlite3 *ptr, int addressbookID, char *href){
	printfunc(__func__);

	char	 			*sql_query = NULL;

	sql_query = sqlite3_mprintf("UPDATE addressbooks SET postURI = %Q WHERE addressbookID = '%d';", href, addressbookID);

	doSimpleRequest(ptr, sql_query, __func__);
}

/**
 * updateContactUri - update the URI of a vCard
 */
void updateContactUri(sqlite3 *ptr, int contactID, char *uri){
	printfunc(__func__);

	char	 			*sql_query = NULL;

	sql_query = sqlite3_mprintf("UPDATE contacts SET href = %Q WHERE contactID = '%d';", uri, contactID);

	doSimpleRequest(ptr, sql_query, __func__);
}

/**
 * updateContact - insert a vCard into the local database
 */
void updateContact(sqlite3 *ptr, int contactID, char *vData){
	printfunc(__func__);

	char	 			*sql_query = NULL;
	char				*dbData = NULL;

	if(!g_str_has_prefix(vData, "BEGIN:VCARD")){
		dbRemoveItem(ptr, "contacts", 2, "", "", "contactID", contactID);
		return;
	}

	sql_query = sqlite3_mprintf("UPDATE contacts SET vCard = %Q WHERE contactID = '%d';", vData, contactID);

	doSimpleRequest(ptr, sql_query, __func__);

	dbData = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", contactID, "", "", "", "","",0);

	if(strlen(dbData) == 1) return;

	setDisplayname(ptr, contactID, dbData);
	g_free(dbData);
}

/**
 * updateUri - update the base URI of a server at the local database
 */
void updateUri(sqlite3 *ptr, int serverID, char *new, gboolean force){
	printfunc(__func__);

	char				*sql_query = NULL;
	char				*old = NULL;
	char				*uri = NULL;
	ne_uri				newUri;
	ne_uri				oldUri;

	old = getSingleChar(ptr, "cardServer", "srvUrl", 1, "serverID", serverID, "", "", "", "", "", 0);

	ne_uri_parse(new, &newUri);
	ne_uri_parse(old, &oldUri);

	if(force == FALSE){
		verboseCC("[%s] %s - %s\n", __func__, oldUri.path, newUri.path);
		if(strlen(oldUri.path) > strlen(newUri.path)){
			verboseCC("[%s] will not update URI\n", __func__);
			return;
		} else {
			verboseCC("[%s] update URI\n", __func__);
		}
	} else {
		verboseCC("[%s] %s%s - %s%s\n", __func__, oldUri.host, oldUri.path, newUri.host, newUri.path);
	}

	newUri.scheme = newUri.scheme ? newUri.scheme : "https";
	newUri.port = newUri.port ? newUri.port : ne_uri_defaultport(newUri.scheme);

	if(newUri.host == NULL){
		uri = g_strdup_printf("%s://%s:%d%s", newUri.scheme, oldUri.host, newUri.port, newUri.path);
	} else {
		uri = g_strdup_printf("%s://%s:%d%s", newUri.scheme, newUri.host, newUri.port, newUri.path);
	}

	sql_query = sqlite3_mprintf("UPDATE cardServer SET srvUrl = %Q WHERE serverID = '%d';", uri, serverID);

	doSimpleRequest(ptr, sql_query, __func__);
	g_free(old);
	g_free(uri);
}

/**
 * checkSyncToken - check how to sync the server
 */
gboolean checkSyncToken(sqlite3 *ptr, int addressbookID){
	printfunc(__func__);

	char			*syncToken = NULL;

	syncToken = getSingleChar(ptr, "addressbooks", "syncToken", 1, "addressbookID", addressbookID, "", "", "", "", "", 0);

	if(strlen(syncToken) == 1) return FALSE;

	if(!strncmp(syncToken, " ", strlen(syncToken))){
		return FALSE;
	}

	g_free(syncToken);
	return TRUE;
}

/**
 * checkAddressbooks - check how to sync the address book
 */
void checkAddressbooks(sqlite3 *ptr, int serverID, int type, ne_session *sess){
	printfunc(__func__);

	GSList			*retList;

	retList = getListInt(ptr, "addressbooks", "addressbookID", 1, "cardServer", serverID, "", "", "", "");

	while(retList) {
		GSList				*next = retList->next;
		int					addressbookID = GPOINTER_TO_INT(retList->data);
		int					syncType;
		ContactCards_stack_t		*stack = NULL;

		if(addressbookID == 0){
			retList = next;
			continue;
		}
		switch(type){
			case 10:
				/*
				 * check how the address book will be synced
				 */
				stack = serverRequest(DAV_REQ_ADDRBOOK_SYNC, serverID, addressbookID, sess, ptr);
				break;
			case 20:
				/*
				 * get the latest content of each address book
				 */
				syncType = getSingleInt(ptr, "addressbooks", "syncMethod", 1, "addressbookID", addressbookID, "", "", "", "");
				if(syncType == -1) return;
				if(syncType & (1<<DAV_ADDRBOOK_DONT_SYNC)){
					goto nextBoook;
				}
				if(syncType & (1<<DAV_SYNC_COLLECTION)){
					if(!checkSyncToken(ptr, addressbookID)){
						stack = serverRequest(DAV_REQ_REP_1, serverID, addressbookID, sess, ptr);
					} else {
						stack = serverRequest(DAV_REQ_REP_2, serverID, addressbookID, sess, ptr);
					}
				} else {
					stack = serverRequest(DAV_REQ_REP_3, serverID, addressbookID, sess, ptr);
				}
				break;
			default:
				verboseCC("[%s] can't handle this number: %d\n", __func__, type);
				return;
		}
		responseHandle(stack, sess, ptr);
		g_free(stack);
nextBoook:
		retList = next;
	}

	g_slist_free(retList);
}

/**
 * updateSyncToken - update the syncing token of an address book
 */
void updateSyncToken(sqlite3 *ptr, int addressbookID, char *syncToken){
	printfunc(__func__);

	char 				*sql_query;

	sql_query = sqlite3_mprintf("UPDATE addressbooks SET syncToken = '%q' WHERE addressbookID = '%d';", syncToken, addressbookID);

	doSimpleRequest(ptr, sql_query, __func__);
}

/**
 * setServerCert - inserts a certificate to a server into the local database
 */
void setServerCert(sqlite3 *ptr, int serverID, int counter, int trustFlag, char *cert, char *digest, char *issued, char *issuer){
	printfunc(__func__);

	char 				*sql_query;

	g_strstrip(cert);
	g_strstrip(digest);
	g_strstrip(issued);
	g_strstrip(issuer);

	if(counter != 0){
		sql_query = sqlite3_mprintf("DELETE FROM certs WHERE serverID = '%d';", serverID);
		doSimpleRequest(ptr, sql_query, __func__);
	}

	sql_query = sqlite3_mprintf("INSERT INTO certs (serverID, trustFlag, digest, issued, issuer, cert) VALUES ('%d','%d','%q','%q','%q','%q');", serverID, trustFlag, digest, issued, issuer, cert);

	doSimpleRequest(ptr, sql_query, __func__);
}

/**
 * updateOAuthCredentials - update the credentials for OAuth at the local database
 */
void updateOAuthCredentials(sqlite3 *ptr, int serverID, int tokenType, char *value){
	printfunc(__func__);

	char 				*sql_query;

	g_strdelimit(value, ",", '\n');
	g_strstrip(value);

	switch(tokenType){
		case OAUTH_ACCESS_GRANT:
			sql_query = sqlite3_mprintf("UPDATE cardServer SET oAuthAccessGrant = '%q' WHERE serverID = '%d';", value, serverID);
			break;
		case OAUTH_ACCESS_TOKEN:
			sql_query = sqlite3_mprintf("UPDATE cardServer SET oAuthAccessToken = '%q' WHERE serverID = '%d';", value, serverID);
			break;
		case OAUTH_REFRESH_TOKEN:
			sql_query = sqlite3_mprintf("UPDATE cardServer SET oAuthRefreshToken = '%q' WHERE serverID = '%d';", value, serverID);
			break;
		default:
			return;
	}

	doSimpleRequest(ptr, sql_query, __func__);
}

/**
 * updateServerFlags - updates the Flags of the server in the database
 */
static void updateServerFlags(int serverID, int flags){
	printfunc(__func__);

	int			old = 0;
	int			diff = 0;

	old = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", serverID, "", "", "", "");

	diff = old & DAV_OPT_MASK;
	if(diff == flags){
		/* Nothing has changed	*/
		verboseCC("[%s] server options hasn't changed\n", __func__);
		return;
	}

	/* Clear the old stuff	*/
	old &= ~DAV_OPT_MASK;
	/* Set the new flags	*/
	old |= flags;
	setSingleInt(appBase.db, "cardServer", "flags", old, "serverID", serverID);
}

/**
 * handleServerOptions - handles the Options returned from a server and update the local database
 */
void handleServerOptions(char *val, int serverID){
	printfunc(__func__);

	gchar			**ptr = NULL;
	int				i = 0;
	int				flags = 0;

	ptr = g_strsplit(val, ",", -1);

	while(ptr[i] != NULL){
		char		*item = NULL;
		item = g_strndup(g_strstrip(ptr[i]), strlen(g_strstrip(ptr[i])));

		if(g_strcmp0(g_ascii_strdown(item, strlen(item)), "post") == 0){
			flags |= DAV_OPT_POST;
		} else if(g_strcmp0(g_ascii_strdown(item, strlen(item)), "put") == 0){
			flags |= DAV_OPT_PUT;
		} else if(g_strcmp0(g_ascii_strdown(item, strlen(item)), "delete") == 0){
			flags |= DAV_OPT_DELETE;
		} else if(g_strcmp0(g_ascii_strdown(item, strlen(item)), "mkcol") == 0){
			flags |= DAV_OPT_MKCOL;
		} else if(g_strcmp0(g_ascii_strdown(item, strlen(item)), "proppatch") == 0){
			flags |= DAV_OPT_PROPPATCH;
		} else if(g_strcmp0(g_ascii_strdown(item, strlen(item)), "move") == 0){
			flags |= DAV_OPT_MOVE;
		} else if(g_strcmp0(g_ascii_strdown(item, strlen(item)), "report") == 0){
			flags |= DAV_OPT_REPORT;
		} else {
			debugCC("[%s] %s\n", __func__, item);
		}
		g_free(item);
		i++;
	}
	g_strfreev(ptr);

	if(flags != 0)
		updateServerFlags(serverID, flags);
}
