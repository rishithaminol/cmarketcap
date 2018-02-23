#include <bson.h>

/*
{
   born : ISODate("1906-12-09"),
   died : ISODate("1992-01-01"),
   name : {
      first : "Grace",
      last : "Hopper"
   },
   languages : [ "MATH-MATIC", "FLOW-MATIC", "COBOL" ],
   degrees: [ { degree: "BA", school: "Vassar" }, { degree: "PhD", school: "Yale" } ]
}
*/

void append_dummy()
{
	bson_t *document;

	document = bson_new();

	bson_append_document(document, "_id", -1, "bitcoin");
	bson_append_utf8(document, "name", -1, "Bitcoin", -1);
	bson_append_utf8(document, "symbol", -1, "BTC", -1);

}

int main(int argc,
  char *     argv[])
{
	struct tm born = { 0 };
	struct tm died = { 0 };
	const char *lang_names[] = { "MATH-MATIC", "FLOW-MATIC", "COBOL" };
	const char *schools[]    = { "Vassar", "Yale" };
	const char *degrees[]    = { "BA", "PhD" };
	uint32_t i;
	char buf[16];
	const char *key;
	size_t keylen;
	bson_t *document;
	bson_t child;
	bson_t child2;
	char *str;

	document = bson_new();

	/*
	 * Append { "born" : ISODate("1906-12-09") } to the document.
	 * Passing -1 for the length argument tells libbson to calculate the string length.
	 */
	born.tm_year = 6;  /* years are 1900-based */
	born.tm_mon  = 11; /* months are 0-based */
	born.tm_mday = 9;
	bson_append_date_time(document, "born", -1, mktime(&born) * 1000);

	/*
	 * Append { "died" : ISODate("1992-01-01") } to the document.
	 */
	died.tm_year = 92;
	died.tm_mon  = 0;
	died.tm_mday = 1;

	/*
	 * For convenience, this macro passes length -1 by default.
	 */
	bson_append_date_time(document, "died", -1, mktime(&died) * 1000);

	/*
	 * Append a subdocument.
	 */
	bson_append_document_begin(document, "name", -1, &child);
	bson_append_utf8(&child, "first", -1, "Grace", -1);
	bson_append_utf8(&child, "last", -1, "Hopper", -1);
	bson_append_document_end(document, &child);

	/*
	 * Append array of strings. Generate keys "0", "1", "2".
	 */
	BSON_APPEND_ARRAY_BEGIN(document, "languages", &child);
	for (i = 0; i < sizeof lang_names / sizeof(char *); ++i) {
		keylen = bson_uint32_to_string(i, &key, buf, sizeof buf);
		bson_append_utf8(&child, key, (int)keylen, lang_names[i], -1);
	}
	bson_append_array_end(document, &child);

	/*
	 * Array of subdocuments:
	 *    degrees: [ { degree: "BA", school: "Vassar" }, ... ]
	 */
	BSON_APPEND_ARRAY_BEGIN(document, "degrees", &child);
	for (i = 0; i < sizeof degrees / sizeof(char *); ++i) {
		keylen = bson_uint32_to_string(i, &key, buf, sizeof buf);
		bson_append_document_begin(&child, key, (int)keylen, &child2);
		BSON_APPEND_UTF8(&child2, "degree", degrees[i]);
		BSON_APPEND_UTF8(&child2, "school", schools[i]);
		bson_append_document_end(&child, &child2);
	}
	bson_append_array_end(document, &child);

	/*
	 * Print the document as a JSON string.
	 */
	str = bson_as_canonical_extended_json(document, NULL);
	printf("%s\n", str);
	bson_free(str);

	/*
	 * Clean up allocated bson documents.
	 */
	bson_destroy(document);
	return 0;
} /* main */
