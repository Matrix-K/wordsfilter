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
    if (! (s = trie_root(trie))) {
        return -1;
    }

    while (*text) {   
        p = text;
        if(! trie_state_is_walkable(s, *p)) {
            trie_state_rewind(s);
            text++;
            continue;
        }

        while(*p && trie_state_is_walkable(s, *p) && ! trie_state_is_leaf(s)) {
            trie_state_walk(s, *p++);  
            if (trie_state_is_terminal(s)) { 
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

PHP_FUNCTION(wordsfilter_search)
{

}

PHP_FUNCTION(wordsfilter_new)
{
	
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
	le_trie_filter = zend_register_list_destructors_ex(
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
	PHP_FE(wordsfilter_new,NULL) 		/* init a words filter instance */
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
