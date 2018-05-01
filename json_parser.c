/**
 * @file json_parser.c
 */

#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <curl/curl.h>
#include <unistd.h>
#include <time.h>

#define COINMARKETCAP_URL "https://api.coinmarketcap.com/v1/ticker/?limit=7000"
#define URL_SIZE          256
#define BUFFER_SIZE       (1024 * 2024) /* 1MB */

#include "json_parser.h"
#include "cm_debug.h"

/* visible only for this section */
static struct coin_entry_base *init_coin_entry_base();
static struct coin_entry *malloc_coin_entry();
static struct coin_entry *mk_coin_entry(json_t *id, json_t *name,
	json_t *symbol, json_t *rank, json_t *price_usd);
static void append_coin_entry(struct coin_entry_base *eb,
	struct coin_entry *ce);
static size_t write_response(void *ptr, size_t size, size_t nmemb,
	void *stream);
static char *request_json_data(const char *url);

/* @brief initialize coin entry base */
static struct coin_entry_base *init_coin_entry_base()
{
	struct coin_entry_base *eb = (struct coin_entry_base *)malloc(
		sizeof(struct coin_entry_base));

	eb->first = NULL;
	eb->last = NULL;
	eb->entry_count = 0;
	eb->derived_time = time(NULL); /* This should be edited */

	return eb;
}

static struct coin_entry *malloc_coin_entry()
{
	struct coin_entry *x = (struct coin_entry *)malloc(sizeof(struct coin_entry));
	if (!x) {
		CM_ERROR("memorry allocation error\n");
		x = NULL;
	}
	return x;
}

/**
 * @brief Creates new 'coin_entry'. Output x->index does not have a value
 * 		  but it is updated inside 'append_coin_entry' using 'entry_count'.
 */
static struct coin_entry *mk_coin_entry(json_t *id, json_t *name,
	json_t *symbol, json_t *rank, json_t *price_usd)
{
	struct coin_entry *x = malloc_coin_entry();
	char *price_usd_str;

	x->id = strdup(json_string_value(id));
	x->name = strdup(json_string_value(name));
	x->symbol = strdup(json_string_value(symbol));
	x->rank = strdup(json_string_value(rank));

	price_usd_str = (char *)json_string_value(price_usd); /* sometimes this returns NULL */
	if (price_usd_str == NULL)
		x->price_usd = NULL;
	else
		x->price_usd = strdup(price_usd_str);

	x->next = NULL;

	return x;
}

/**
 * @brief append the given coin entry at the end of the linked list 
 *
 *	@param[in] eb entry_base
 *
 *	@param[in] cd coin_entry
 */
static void append_coin_entry(struct coin_entry_base *eb, struct coin_entry *ce)
{
	if (eb->first == NULL) {/* first and last NULL */
		eb->first = ce;
		eb->last = eb->first; /* initially first == last */
	} else {
		eb->last->next = ce;
		eb->last = eb->last->next;
	}

	eb->last->index = eb->entry_count++;
}

/**
 * @brief Print the given 'coin_entry_base' structure
 */
void print_entries(struct coin_entry_base *eb)
{
	DEBUG_MSG("number of entries = %d\n", eb->entry_count);
	DEBUG_MSG("Derived time = %s\n", "Will be implemented soon");

	struct coin_entry *t;

	t = eb->first;

	while (t != NULL) {
		printf("%s:%s:%s:%s:%s\n", t->id, t->name, t->symbol,
			t->rank, t->price_usd);
		t = t->next;
	}
}

/**
 * @brief freeup entire 'coin_entry_base' C data structure
 *
 * @return Nothing
 */
void free_entry_base(struct coin_entry_base *eb)
{
	struct coin_entry *entry, *entry2;

	entry = eb->first;
	free(eb);

	while (entry != NULL) {
		free(entry->id);
		free(entry->name);
		free(entry->symbol);
		free(entry->rank);
		free(entry->price_usd);

		entry2 = entry->next;
		free(entry);
		entry = entry2;
	}
}

/**
 * @brief Downloads cryptocurrency data from 'coinmarketcap.com'
 *		  and return them as a C data structure.
 *
 * @detail In this program we call this structure as 'coin_entry_base'
 *
 * @return 'coin_entry_base'
 */
struct coin_entry_base *new_coin_entry_base()
{
	size_t i, a_size;
	char *json_text;
	char url[URL_SIZE];

	json_t *root;
	json_error_t error;

	struct coin_entry_base *entry_base = init_coin_entry_base();

	snprintf(url, URL_SIZE, COINMARKETCAP_URL);

	json_text = request_json_data(url);
	if (!json_text)
		return NULL;

	root = json_loads(json_text, 0, &error);
	free(json_text);

	if (!root) {
		CM_ERROR("error: on line %d: %s\n", error.line, error.text);
		return NULL;
	}

	if (!json_is_array(root)) {
		CM_ERROR("root is not an array\n");
		json_decref(root);
		return NULL;
	}

	a_size = json_array_size(root);
	for (i = 0; i < a_size; i++) {
		json_t *data, *id, *name, *symbol, *rank, *price_usd;
		struct coin_entry *entry;

		data = json_array_get(root, i);

		id        = json_object_get(data, "id");
		name      = json_object_get(data, "name");
		symbol    = json_object_get(data, "symbol");
		rank      = json_object_get(data, "rank");
		price_usd = json_object_get(data, "price_usd");

		entry = mk_coin_entry(id, name, symbol, rank, price_usd);
		append_coin_entry(entry_base, entry);
	}

	json_decref(root);

	return entry_base;
}

/* @brief used by 'write_reponse()' and 'request_json_data()' */
struct write_result {
	char *data;
	int   pos;
};

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct write_result *result = (struct write_result *)stream;

	if (result->pos + size * nmemb >= BUFFER_SIZE - 1) {
		CM_ERROR("too small buffer\n");
		return 0;
	}

	memcpy(result->data + result->pos, ptr, size * nmemb);
	result->pos += size * nmemb;

	return size * nmemb;
}

/**
 * @brief This will return a large text output which is needed
 *		  by 'json_loads()'
 *
 * @param[in] url API url to be read from 'coinmarketcap.com'.
 *
 * @return Returns fetched data as a raw string.
 */
static char *request_json_data(const char *url)
{
	CURL *curl = NULL;
	CURLcode status;
	struct curl_slist *headers = NULL;
	char *data = NULL;
	long code;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (!curl)
		goto error;

	data = malloc(BUFFER_SIZE);
	if (!data)
		goto error;

	struct write_result write_result = {
		.data = data,
		.pos  = 0
	};

	curl_easy_setopt(curl, CURLOPT_URL, url);

	/* GitHub commits API v3 requires a User-Agent header */
	headers = curl_slist_append(headers, "User-Agent: Jansson-Tutorial");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

	while ((status = curl_easy_perform(curl)) != 0) {
		CM_ERROR("unable to request data from %s:\n", url);
		CM_ERROR("%s\n", curl_easy_strerror(status));
		sleep(1);
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if (code != 200) {
		CM_ERROR("server responded with code %ld\n", code);
		goto error;
	}

	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);
	curl_global_cleanup();

	/* zero-terminate the result */
	data[write_result.pos] = '\0';

	return data;

error:
	if (data)
		free(data);
	if (curl)
		curl_easy_cleanup(curl);
	if (headers)
		curl_slist_free_all(headers);
	curl_global_cleanup();
	return NULL;
} /* request */
