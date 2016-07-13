/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_wordsfilter.h"

/* If you declare any globals in php_wordsfilter.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(wordsfilter)
*/

/* True global resources - no need for thread safety here */
static int le_wordsfilter;

static void php_trie_filter_dtor(zend_resource *rsrc TSRMLS_DC)
{
	Trie *trie = (Trie *)rsrc->ptr;
	trie_free(trie);
}

static int trie_search_one(Trie *trie, const AlphaChar *text, int *offset, TrieData *length)
{
	TrieState *s;
	const AlphaChar *p;
	const AlphaChar *base;

	base = text;
    	if (! (s = trie_root(trie))) {
        	return -1;
    	}

	while (*text) {		
		p = text;
		if (! trie_state_is_walkable(s, *p)) {
            		trie_state_rewind(s);
			text++;
			continue;
		} else {
			trie_state_walk(s, *p++);
        	}

		while (trie_state_is_walkable(s, *p) && ! trie_state_is_terminal(s))
			trie_state_walk(s, *p++);

		if (trie_state_is_terminal(s)) {
			*offset = text - base;
			*length = p - text;
            		trie_state_free(s);
            
			return 1;
		}

        	trie_state_rewind(s);
		text++;
	}
    	trie_state_free(s);

	return 0;
}

static int trie_search_all(Trie *trie, const AlphaChar *text, zval *data)
{
	TrieState *s;
	const AlphaChar *p;
	const AlphaChar *base;
    	zval *word = NULL;

	base = text;
    	if (! (s = trie_root(trie))) 
    	{
        	return -1;
    	}

    	while (*text) 
    	{   
        	p = text;
        	if(! trie_state_is_walkable(s, *p)) 
        	{
            		trie_state_rewind(s);
            		text++;
            		continue;
        	}

        	while(*p && trie_state_is_walkable(s, *p) && ! trie_state_is_leaf(s)) 
        	{
            		trie_state_walk(s, *p++);  
            		if (trie_state_is_terminal(s)) 
            		{ 
                		zval *word;
				array_init_size(word, 3);
                		add_next_index_long(word, text - base);
                		add_next_index_long(word, p - text);
                		add_next_index_zval(data, word);        
            		}        
        	}
        	trie_state_rewind(s);
        	text++;
    	}
    	trie_state_free(s);

	return 0;
}

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("wordsfilter.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_wordsfilter_globals, wordsfilter_globals)
    STD_PHP_INI_ENTRY("wordsfilter.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_wordsfilter_globals, wordsfilter_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

PHP_FUNCTION(wordsfilter_create_instance)
{
    Trie *trie;
    AlphaMap *alpha_map;
    int ret;

    alpha_map = alpha_map_new();
    if (! alpha_map) {
        RETURN_NULL();
    }

    if (alpha_map_add_range(alpha_map, 0x00, 0xff) != 0) {
        /* treat all strings as byte stream */
        alpha_map_free(alpha_map);
        RETURN_NULL();
    }

    trie = trie_new(alpha_map);
    alpha_map_free(alpha_map);
    if (! trie) {      
        RETURN_NULL();
    }

    RETURN_RES(zend_register_resource(trie,le_wordsfilter));
}

PHP_FUNCTION(wordsfilter_create_word)
{
    Trie *trie;
    zval *trie_resource;
    char *keyword, *p;
    int keyword_len, i;
    AlphaChar alpha_key[KEYWORD_MAX_LEN+1];
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", 
				&trie_resource, &keyword, &keyword_len) == FAILURE) 
    {
    	RETURN_FALSE;
    }

    if (keyword_len > KEYWORD_MAX_LEN || keyword_len < 1) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "keyword should has [1, %d] bytes", KEYWORD_MAX_LEN);
        RETURN_FALSE;
    }

    if((trie = (Trie *)zend_fetch_resource(Z_RES_P(trie_resource),PHP_TRIE_FILTER_RES_NAME,le_wordsfilter)) == NULL)
    {
    	RETURN_FALSE;
    }

	// zend_fetch_resource(&trie_resource, PHP_TRIE_FILTER_RES_NAME , -1);
    // zend_fetch_resource(trie, Trie *, &trie_resource, -1, PHP_TRIE_FILTER_RES_NAME, le_wordsfilter);

    p = keyword;
    i = 0;
    while (*p && *p != '\n' && *p != '\r') {
        alpha_key[i++] = (AlphaChar)*p;
        p++;
    }
    alpha_key[i] = TRIE_CHAR_TERM;
    if (! trie_store(trie, alpha_key, -1)) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

PHP_FUNCTION(wordsfilter_save)
{
    Trie *trie;
    zval *trie_resource;
    char *filename;
    int filename_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", 
                &trie_resource, &filename, &filename_len) == FAILURE) {
        RETURN_FALSE;
    }
    if (filename_len < 1 || strlen(filename) != filename_len) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "save path required");
        RETURN_FALSE;
    }
    if((trie = (Trie *)zend_fetch_resource(Z_RES_P(trie_resource),PHP_TRIE_FILTER_RES_NAME,le_wordsfilter)) == NULL)
    {
    	RETURN_FALSE;
    }
    // ZEND_FETCH_RESOURCE(trie, Trie *, &trie_resource, -1, PHP_TRIE_FILTER_RES_NAME, le_trie_filter);
    if (trie_save(trie, filename)) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

PHP_FUNCTION(wordsfilter_search)
{
	Trie *trie;
	zval *trie_resource;
	char *text;
	int text_len;

	int offset = -1, i, ret;
    TrieData length = 0;

	AlphaChar *alpha_text;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", 
				&trie_resource, &text, &text_len) == FAILURE) {
		RETURN_FALSE;
	}

    array_init(return_value);
    if (text_len < 1 || strlen(text) != text_len) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "input is empty");
		return;
	}

	if((trie = (Trie *)zend_fetch_resource(Z_RES_P(trie_resource),PHP_TRIE_FILTER_RES_NAME,le_wordsfilter)) == NULL)
    {
    	RETURN_FALSE;
    }

	alpha_text = emalloc(sizeof(AlphaChar) * (text_len + 1));

	for (i = 0; i < text_len; i++) {
		alpha_text[i] = (AlphaChar) text[i];
	}

	alpha_text[text_len] = TRIE_CHAR_TERM;

	ret = trie_search_one(trie, alpha_text, &offset, &length);
    efree(alpha_text);
	if (ret == 0) {
        return;
    } else if (ret == 1) {
		add_next_index_long(return_value, offset);
		add_next_index_long(return_value, length);
	} else {
        RETURN_FALSE;
    }
}

PHP_FUNCTION(wordsfilter_search_all)
{
	Trie *trie;
	zval *trie_resource;
	char *text;
	int text_len;

	int i, ret;

	AlphaChar *alpha_text;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", 
				&trie_resource, &text, &text_len) == FAILURE) {
		RETURN_FALSE;
	}

    array_init(return_value);

    if (text_len < 1 || strlen(text) != text_len) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "input is empty");
		return;
	}

	if((trie = (Trie *)zend_fetch_resource(Z_RES_P(trie_resource),PHP_TRIE_FILTER_RES_NAME,le_wordsfilter)) == NULL)
    {
    	RETURN_FALSE;
    }
	// zend_fetch_resource(&trie_resource, PHP_TRIE_FILTER_RES_NAME , -1);

	alpha_text = emalloc(sizeof(AlphaChar) * (text_len + 1));

	for (i = 0; i < text_len; i++) {
		alpha_text[i] = (AlphaChar) text[i];
	}

	alpha_text[text_len] = TRIE_CHAR_TERM;

	ret = trie_search_all(trie, alpha_text, return_value);
    efree(alpha_text);
	if (ret == 0) {
        return;
	} else {
        RETURN_FALSE;
    }
}

PHP_FUNCTION(wordsfilter_load)
{
	Trie *trie;
	char *path;
	int path_length;
	/* receive params */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", 
				&path, &path_length) == FAILURE) {
		RETURN_NULL();
	}

	trie = trie_new_from_file(path);

	if(!trie)
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to load %s", path);
		RETURN_NULL();
	}

	RETURN_RES(zend_register_resource(trie,le_wordsfilter));
}

PHP_FUNCTION(wordsfilter_free)
{
	Trie *trie;
    zval *trie_resource;
    int resource_id;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &trie_resource) == FAILURE) {
        RETURN_FALSE;
    }

    if((trie = (Trie *)zend_fetch_resource(Z_RES_P(trie_resource),PHP_TRIE_FILTER_RES_NAME,le_wordsfilter)) == NULL)
    {
    	RETURN_FALSE;
    }
    // ZEND_FETCH_RESOURCE(trie, Trie *, &trie_resource, -1, PHP_TRIE_FILTER_RES_NAME, le_trie_filter);
    resource_id = Z_RES_P(trie_resource)->handle;
    if (zend_list_close(Z_RES_P(trie_resource)) == SUCCESS) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_wordsfilter_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_wordsfilter_init_globals(zend_wordsfilter_globals *wordsfilter_globals)
{
	wordsfilter_globals->global_value = 0;
	wordsfilter_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(wordsfilter)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
	le_wordsfilter = zend_register_list_destructors_ex(
			php_trie_filter_dtor, 
			NULL, PHP_TRIE_FILTER_RES_NAME, module_number);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(wordsfilter)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(wordsfilter)
{
#if defined(COMPILE_DL_WORDSFILTER) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(wordsfilter)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(wordsfilter)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "wordsfilter support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ wordsfilter_functions[]
 *
 * Every user visible function must have an entry in wordsfilter_functions[].
 */
const zend_function_entry wordsfilter_functions[] = {
	// PHP_FE(confirm_wordsfilter_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(wordsfilter_search,NULL) 	/* search words in text */
	PHP_FE(wordsfilter_load,NULL) 		/* init a words filter instance */
	PHP_FE(wordsfilter_search_all,NULL) /* search all keywords or sensitive words in text*/
	PHP_FE(wordsfilter_save,NULL) /* save a trie file */
	PHP_FE(wordsfilter_free,NULL) /* free a trie file*/
	PHP_FE(wordsfilter_create_instance,NULL) /* create a new instance of trie */
	PHP_FE(wordsfilter_create_word,NULL) /* add a new keywords or sensitive word to trie tree */
	PHP_FE_END	/* Must be the last line in wordsfilter_functions[] */
};
/* }}} */

/* {{{ wordsfilter_module_entry
 */
zend_module_entry wordsfilter_module_entry = {
	STANDARD_MODULE_HEADER,
	"wordsfilter",
	wordsfilter_functions,
	PHP_MINIT(wordsfilter),
	PHP_MSHUTDOWN(wordsfilter),
	PHP_RINIT(wordsfilter),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(wordsfilter),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(wordsfilter),
	PHP_WORDSFILTER_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_WORDSFILTER
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(wordsfilter)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
