#ifndef PHP_FCGX_H
#define PHP_FCGX_H 1

#define PHP_FCGX_VERSION "0.1"
#define PHP_FCGX_EXTNAME "fcgx"

PHP_MINIT_FUNCTION(fcgx);

PHP_FUNCTION(FCGX_Listen);
PHP_FUNCTION(FCGX_Accept);
//PHP_FUNCTION(FCGX_Flush);
PHP_FUNCTION(FCGX_Finish);
PHP_FUNCTION(FCGX_Hello_World);

extern zend_module_entry hello_module_entry;
#define phpext_hello_ptr &hello_module_entry

#endif
