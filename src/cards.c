/*
 *	cards.c
 */

#include "ContactCards.h"

/**
 * getUID - create a UID for a new vCard
 *
 * Based on the comment by Andrew Moore on
 * http://www.php.net/manual/en/function.uniqid.php#94959
 */
static char *getUID(void){
	printfunc(__func__);

	char		*uid = NULL;
	GRand		*rand = g_rand_new();

	uid = g_strdup_printf("%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
		g_rand_int_range(rand, 0, 0xffff),
		g_rand_int_range(rand, 0, 0xffff),
		g_rand_int_range(rand, 0, 0xffff),
		g_rand_int_range(rand, 0, 0x0fff) | 0x4000,
		g_rand_int_range(rand, 0, 0x3fff) | 0x8000,
		g_rand_int_range(rand, 0, 0xffff),
		g_rand_int_range(rand, 0, 0xffff),
		g_rand_int_range(rand, 0, 0xffff));

	return uid;
}

/**
 * buildAdr - create the address field of a vCard
 * 
 * RFC 2426 - 3.2.1 ADR Type Definition:
 *
 * The structured type value corresponds, in sequence, to
 * the post office box;
 * the extended address;
 * the street address;
 * the locality (e.g., city);
 * the region (e.g., state or province);
 * the postal code;
 * the country name.
 */
static char *buildAdr(GSList *list){
	printfunc(__func__);

	char				*adr = "";
	char				*poBox = NULL;
	char				*extAdr = NULL;
	char				*str = NULL;
	char				*loc = NULL;
	char				*reg = NULL;
	char				*zip = NULL;
	char				*country = NULL;
	GSList				*next;
	GString				*tmp;

	tmp = g_string_new(NULL);

	g_string_append(tmp, "ADR");

	while(list){
		ContactCards_item_t		*item;
		next = list->next;

		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		switch(item->itemID){
			case CARDTYPE_ADR_OPT:
				if(!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)))
					break;
				g_string_append(tmp,";TYPE=");
				g_string_append(tmp, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)));
				break;
			case CARDTYPE_ADR_OFFICE_BOX:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				poBox = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_EXT_ADDR:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				extAdr = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_STREET:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				str = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_CITY:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				loc = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_REGION:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				reg = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_ZIP:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				zip = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_COUNTRY:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				country = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
		}
stepForward:
		list = next;
	}

	if(!poBox && !extAdr && !str && !loc && !reg && !zip && !country)
		goto stepEmpty;

	g_string_append(tmp, ":");
	if(poBox)
		g_string_append(tmp, poBox);
	g_string_append(tmp, ";");
	if(extAdr)
		g_string_append(tmp, extAdr);
	g_string_append(tmp, ";");
	if(str)
		g_string_append(tmp, str);
	g_string_append(tmp, ";");
	if(loc)
		g_string_append(tmp, loc);
	g_string_append(tmp, ";");
	if(reg)
		g_string_append(tmp, reg);
	g_string_append(tmp, ";");
	if(zip)
		g_string_append(tmp, zip);
	g_string_append(tmp, ";");
	if(country)
		g_string_append(tmp, country);
	g_string_append(tmp, "\r\n");
	adr = g_strndup(tmp->str, tmp->len);

	g_string_free(tmp, TRUE);
	free(poBox);
	free(extAdr);
	free(str);
	free(loc);
	free(reg);
	free(zip);
	free(country);

stepEmpty:
	return adr;
}

/**
 * buildUrl - extract the Url of the user input
 */
static char *buildUrl(GSList *list){
	printfunc(__func__);

	char				*url = "";
	GSList				*next;
	GString				*tmp;

	tmp = g_string_new(NULL);

	g_string_append(tmp, "URL");

	while(list){
		ContactCards_item_t		*item;
		next = list->next;

		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		switch(item->itemID){
			case CARDTYPE_EMAIL_OPT:
				if(!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)))
					break;
				g_string_append(tmp,";TYPE=");
				g_string_append(tmp, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)));
				break;
			default:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					goto stepEmpty;
				g_string_append(tmp,":");
				g_string_append(tmp, gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER(item->element)));
		}
stepForward:
		list = next;
	}

	g_string_append(tmp, "\n");
	url = g_strndup(tmp->str, tmp->len);

stepEmpty:
	return url;
}

/**
 * buildEMail - extract the EMail of the user input
 */
static char *buildEMail(GSList *list){
	printfunc(__func__);

	char				*mail = "";
	GSList				*next;
	GString				*tmp;

	tmp = g_string_new(NULL);

	g_string_append(tmp, "EMAIL");

	while(list){
		ContactCards_item_t		*item;
		next = list->next;

		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		switch(item->itemID){
			case CARDTYPE_EMAIL_OPT:
				if(!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)))
					break;
				g_string_append(tmp,";TYPE=");
				g_string_append(tmp, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)));
				break;
			default:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					goto stepEmpty;
				g_string_append(tmp,":");
				g_string_append(tmp, gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER(item->element)));
		}
stepForward:
		list = next;
	}

	g_string_append(tmp, "\n");
	mail = g_strndup(tmp->str, tmp->len);

	g_string_free(tmp, TRUE);

stepEmpty:
	return mail;
}

/**
 * buildTele - extract the  telephone number of the user input
 */
static char *buildTele(GSList *list){
	printfunc(__func__);

	char				*tel = "";
	GSList				*next;
	GString				*tmp;

	tmp = g_string_new(NULL);

	g_string_append(tmp, "TEL");

	while(list){
		ContactCards_item_t		*item;
		next = list->next;

		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		switch(item->itemID){
			case CARDTYPE_TEL_OPT:
				if(!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)))
					break;
				g_string_append(tmp,";TYPE=");
				g_string_append(tmp, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)));
				break;
			default:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					goto stepEmpty;
				g_string_append(tmp,":");
				g_string_append(tmp, gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER(item->element)));
		}
stepForward:
		list = next;
	}

	g_string_append(tmp, "\n");
	tel = g_strndup(tmp->str, tmp->len);

	g_string_free(tmp, TRUE);

stepEmpty:
	return tel;
}

/**
 * buildCard - create a RFC conform vCard from the user input
 */
char *buildCard(GSList *list){
	printfunc(__func__);

	char				*card = NULL;
	char				*firstN , *lastN, *middleN, *prefixN, *suffixN;
	int					bDay, bMonth, bYear;
	char				*bDate;
	GSList				*next;
	GString				*cardString;

	cardString = g_string_new(NULL);

	firstN = lastN = middleN = prefixN = suffixN = NULL;

	g_string_append(cardString, "BEGIN:VCARD\r\n");
	g_string_append(cardString, "VERSION:3.0\r\n");

	g_string_append(cardString, "PRODID:-//ContactCards//ContactCards");
	g_string_append(cardString, VERSION);
	g_string_append(cardString, "//EN\r\n");
	g_string_append(cardString, "UID:");
	g_string_append(cardString, getUID());
	g_string_append(cardString, "\r\n");

	while(list){
		ContactCards_item_t		*item;
		next = list->next;

		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		switch(item->itemID){
			case CARDTYPE_ADR:
				g_string_append(cardString, buildAdr(item->element));
				break;
			case CARDTYPE_TEL:
				g_string_append(cardString, buildTele(item->element));
				break;
			case CARDTYPE_FN_FIRST:
				firstN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_LAST:
				lastN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_PREFIX:
				prefixN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_MIDDLE:
				middleN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_SUFFIX:
				suffixN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_BDAY:
				gtk_calendar_get_date(GTK_CALENDAR(item->element), &bYear, &bMonth, &bDay);
				bMonth++;
				bDate = g_strdup_printf("%04d-%02d-%02d", bYear, bMonth, bDay);
				g_string_append(cardString, "BDAY:");
				g_string_append(cardString, bDate);
				g_string_append(cardString, "\r\n");
				break;
			case CARDTYPE_EMAIL:
				g_string_append(cardString, buildEMail(item->element));
				break;
			case CARDTYPE_URL:
				g_string_append(cardString, buildUrl(item->element));
				break;
			default:
				break;
		}
stepForward:
		list = next;
	}

/*
 * RFC 2426 - 3.1.2 N Type Definition :
 *
 * The structured type value corresponds, in sequence, to
 * the Family Name,
 * Given Name,
 * Additional Names,
 * Honorific Prefixes, and
 * Honorific Suffixes
 */
	g_string_append(cardString, "N:");
	g_string_append(cardString, lastN);
	g_string_append(cardString, ";");
	g_string_append(cardString, firstN);
	g_string_append(cardString, ";");
	g_string_append(cardString, middleN);
	g_string_append(cardString, ";");
	g_string_append(cardString, prefixN);
	g_string_append(cardString, ";");
	g_string_append(cardString, suffixN);
	g_string_append(cardString, "\r\n");

	g_string_append(cardString, "FN:");
	g_string_append(cardString, prefixN);
	g_string_append(cardString, " ");
	g_string_append(cardString, firstN);
	g_string_append(cardString, " ");
	g_string_append(cardString, middleN);
	g_string_append(cardString, " ");
	g_string_append(cardString, lastN);
	g_string_append(cardString, " ");
	g_string_append(cardString, suffixN);
	g_string_append(cardString, "\r\n");

	g_string_append(cardString, "END:VCARD\n");

	card = g_strndup(cardString->str, cardString->len);

	free(firstN);
	free(lastN);
	g_string_free(cardString, TRUE);

	return card;
}

/**
 * getAttributValue - split the line of a vCard in its elements and return the value
 */
static char *getAttributValue(char *line){
	printfunc(__func__);

	char				**elements = g_strsplit(line, ":", 2);
	char				**element = elements;
	char				*value = NULL;

	value = element[1];

	return value;
}

/**
 * getMultipleCardAttribut - return a vCard element which can occur multiple times
 */
GSList *getMultipleCardAttribut(int type, char *card){
	printfunc(__func__);

	GSList				*list = g_slist_alloc();
	char				**lines = g_strsplit(card, "\n", 0);
	char				**line = lines;
	char				*value = NULL;

	while (*line != NULL) {
		switch(type){
			case CARDTYPE_ADR:
				if(g_regex_match_simple ("[^item]?ADR", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_TEL:
				if(g_regex_match_simple ("[^item]?TEL", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_URL:
				if(g_regex_match_simple ("[^item]?URL", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_EMAIL:
				if(g_regex_match_simple ("[^item]?EMAIL", *line, 0,0))
					goto getValue;
				else
					goto next;
			default:
				goto next;
		}
		getValue:
			value = getAttributValue(*line);
			list = g_slist_append(list, value);
		next:
			line++;
	}

	return list;
}

/**
 * getCardPhoto - return the photo of a vCard
 */
ContactCards_pix_t *getCardPhoto(char *card){
	printfunc(__func__);

	char				*start = g_strrstr(card, "PHOTO");
	GString				*buf;
	GString				*tmp;
	guchar				*pix = NULL;
	gsize				len;
	int					i = 0;
	int					j = 0;
	ContactCards_pix_t	*pic;

	pic = g_new(ContactCards_pix_t,1);

	pic->pixel	= NULL;
	pic->size	= 0;

	if(start == NULL){
		return pic;
	}

	while(start[i] != ':')
		i++;
	i++;	/* Set i to the point after the :	*/

	buf = g_string_new(NULL);
	tmp = g_string_new(NULL);

	while(start[i+j] != ':')
		g_string_append_unichar(buf, start[i+(j++)]);

	/* Remove unnecessary stuff	*/
	i = buf->len;

	while(buf->str[i] != '\n')
		i--;
	g_string_truncate(buf, i);

	i = 0;
	j = 1;
	while(buf->str[i])
	{
		switch(buf->str[i]){
			case 43:			/*	+		*/
			case 47:			/*	/		*/
			case 48 ... 57:		/*	0-9		*/
			case 61:			/*	=		*/
			case 65 ... 90:		/*	A-Z		*/
			case 97 ... 122:	/*	a-z		*/
				g_string_append_unichar(tmp, buf->str[i]);
				if(j%79 == 0 && j != 1)
					g_string_append_unichar(tmp, '\n');
				break;
			default:
				break;
		}
		i++;
		j++;
	}
	pix = g_base64_decode(tmp->str, &len);
	pic->pixel = pix;
	pic->size = (int) len;

	g_string_free(buf, TRUE);
	g_string_free(tmp, TRUE);

	return pic;
}

/**
 * getSingleCardAttribut - return a vCard element which can occur only once
 */
char *getSingleCardAttribut(int type, char *card){
	printfunc(__func__);

	char						**lines = g_strsplit(card, "\n", 0);
	char						**line = lines;
	char						*value = NULL;

	while (*line != NULL) {
		switch(type){
			case CARDTYPE_FN:
				if(g_str_has_prefix(*line, "FN"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_N:
				if(g_str_has_prefix(*line, "N:"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_NICKNAME:
				if(g_str_has_prefix(*line, "NICKNAME"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_LABEL:
				if(g_str_has_prefix(*line, "LABEL"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_CLASS:
				if(g_str_has_prefix(*line, "CLASS"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_KEY:
				if(g_str_has_prefix(*line, "KEY"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_UID:
				if(g_str_has_prefix(*line, "UID"))
					goto getValue;
				else
					goto next;
			case CARDTYPE_BDAY:
				if(g_str_has_prefix(*line, "BDAY"))
					goto getValue;
				else
					goto next;
			default:
				goto next;
		}
		getValue:
			value = getAttributValue(*line);
			break;
		next:
			line++;
	}

	return value;
}

/**
 * replaceAntiquatedLine - replaces a line of a vCard with a new value
 * This function only works for values which occur only once
 */
static char *replaceAntiquatedLine(char *vCard, char *pattern, char *newLine){
	printfunc(__func__);

	GString		*data;
	const char	*end;
	const char *p = vCard;

	unsigned int	i = 0;
	int			k = 0;
	int			sPos = 0;

	data = g_string_new(NULL);
	data = g_string_assign(data, vCard);

	end = vCard + strlen(vCard) - strlen(pattern);

	sPos = 0;
	while(p <= end && *p){
		for(i=0; i < strlen(pattern); i++){
			if(p[i] != pattern[i]){
				goto nextLoop;
			}
		}
		break;
nextLoop:
		p++;
		sPos++;
	}

	if(i == (strlen(vCard) - strlen(pattern))){
		dbgCC("[%s] didn't find %s\n", __func__, pattern);
		return NULL;
	}
	if(pattern[0] == '\n')
		sPos++;

	k= 0;
	while(vCard[sPos + k] != '\n')
		k++;
	k++;

	g_string_erase(data, sPos, k);

	g_string_insert(data, sPos, newLine);

	return data->str;
}
/**
 * mergeCards - merge the old vCard from the database with the new changes
 * This function is needed to keep the stuff alive which is not
 * displayed and editable so far
 */
char *mergeCards(GSList *new, char *old){
	printfunc(__func__);

	char			*firstN , *lastN, *middleN, *prefixN, *suffixN;
	GSList			*next;
	GString			*value;

	firstN = lastN = middleN = prefixN = suffixN = NULL;

	while(new){
		ContactCards_item_t		*item;
		next = new->next;
		if(!new->data)
			goto stepForward;
		item = (ContactCards_item_t *)new->data;
		value = g_string_new(NULL);
		switch(item->itemID){
			case CARDTYPE_FN_FIRST:
				firstN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_LAST:
				lastN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_PREFIX:
				prefixN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_MIDDLE:
				middleN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_SUFFIX:
				suffixN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			default:
				break;
		}
		g_string_free(value, TRUE);
stepForward:
		new = next;
	}

	value = g_string_new(NULL);
	g_string_append(value, "N:");
	g_string_append(value, lastN);
	g_string_append(value, ";");
	g_string_append(value, firstN);
	g_string_append(value, ";");
	g_string_append(value, middleN);
	g_string_append(value, ";");
	g_string_append(value, prefixN);
	g_string_append(value, ";");
	g_string_append(value, suffixN);
	g_string_append(value, "\r\n");

	if(g_strstr_len(old, -1, value->str) == NULL)
	{
		old = replaceAntiquatedLine(old, "\nN:", value->str);
		g_string_free(value, TRUE);

		value = g_string_new(NULL);
		g_string_append(value, "FN:");
		g_string_append(value, prefixN);
		g_string_append(value, " ");
		g_string_append(value, firstN);
		g_string_append(value, " ");
		g_string_append(value, middleN);
		g_string_append(value, " ");
		g_string_append(value, lastN);
		g_string_append(value, " ");
		g_string_append(value, suffixN);
		g_string_append(value, "\r\n");

		old = replaceAntiquatedLine(old, "\nFN:", value->str);
		g_string_free(value, TRUE);
	}

	value = g_string_new(NULL);
	g_string_append(value, "PRODID:-//ContactCards//ContactCards");
	g_string_append(value, VERSION);
	g_string_append(value, "//EN\r\n");
	old = replaceAntiquatedLine(old, "\nPRODID:", value->str);
	g_string_free(value, TRUE);

	return old;
}
