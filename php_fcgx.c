/* ==== PHP Module definitions ==== */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include "php_fcgx.h"

static zend_function_entry fcgx_functions[] = {
    PHP_FE(FCGX_Listen, NULL)
    PHP_FE(FCGX_Accept, NULL)
    PHP_FE(FCGX_Finish, NULL)
    PHP_FE(FCGX_Hello_World, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry fcgx_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_FCGX_EXTNAME,
    fcgx_functions,
    PHP_MINIT(fcgx),
    NULL,
    NULL,
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    PHP_FCGX_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_FCGX
ZEND_GET_MODULE(fcgx)
#endif

#include <fcgiapp.h>

/**
 * API:
 * FCGX_Listen("address"): listen on this address instead of stdio
 * FCGX_Accept: Accept a request
 * 
 * Two options:
 * return an object from FCGX_Accept:
 * ->in
 * ->out
 * ->err : streams for in/out/err, destroyed when finished
 * ->env : environment from fcgi
 * ->finish : finishes the request
 * 
 * Second option:
 * same as above but keep functions seperate from the object
 * 
 */


/* ==== Stream Opperatins ==== */

size_t php_fcgx_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC);
size_t php_fcgx_read (php_stream *stream, char *buf,       size_t count TSRMLS_DC);
int    php_fcgx_close(php_stream *stream, int close_handle TSRMLS_DC);
int    php_fcgx_flush(php_stream *stream TSRMLS_DC);

PHPAPI php_stream_ops php_fcgx_stream_ops = {
	.write = php_fcgx_write,
	.read  = php_fcgx_read,
	.close = php_fcgx_close,
	.flush = php_fcgx_flush,
	.seek  = NULL,
	.cast  = NULL,
	.stat  = NULL,
	.set_option = NULL
};

size_t php_fcgx_write(php_stream *_stream, const char *buf, size_t count TSRMLS_DC)
{
	FCGX_Stream *stream = (FCGX_Stream*)_stream->abstract;
	size_t ret = 0;
	//printf("Writing to fcgx\n");
	// v is actually write(), doesnt stop on null
	ret = FCGX_PutStr(buf, count, stream);
	return ret;
}

size_t php_fcgx_read (php_stream *_stream, char *buf, size_t count TSRMLS_DC)
{
	FCGX_Stream *stream = (FCGX_Stream*)_stream->abstract;
	size_t ret = 0;
	//printf("Reading from fcgx\n");
	// v is actually read()
	ret = FCGX_GetStr(buf, count, stream);
	return ret;
}

//dafaq is close_handle
int php_fcgx_close(php_stream *_stream, int close_handle TSRMLS_DC)
{
	FCGX_Stream *stream = (FCGX_Stream*)_stream->abstract;
	int ret = 0;
	//printf("Closing fcgx stream\n");
	//ret = FCGX_FClose(stream);
	return ret;
}

int php_fcgx_flush(php_stream *_stream TSRMLS_DC)
{
	FCGX_Stream *stream = (FCGX_Stream*)_stream->abstract;
	int ret = 0;
	//printf("Flushing fcgx\n");
	//ret = FCGX_FFlush(stream);
	return ret;
}


static php_stream *php_stream_from_fcgx(FCGX_Stream *stream, const char *mode)
{
	//printf("Fcgx to zval\n");
	return php_stream_alloc_rel(&php_fcgx_stream_ops, stream, NULL, mode);
}

/* ==== End Stream Opperatins ==== */

/* ==== Class Definition ==== */
ZEND_BEGIN_ARG_INFO(arginfo_null, 0)
ZEND_END_ARG_INFO()

zend_class_entry *fcgx_entry;

typedef struct _FCGXReq {
	zval *in, *out, *err, *env;
	FCGX_Request request;
} FCGXReq;

PHP_METHOD(FCGXRequest, finish) {
	FCGX_Finish_r(&priv.request);
}

PHP_METHOD(FCGXRequest, helloWorld) {
{
	zval *object = getThis():
	FCGX_FPrintF(priv.request.out, "HTTP 200 Ok\r\nContent-type: text/html\r\n\r\n <h1>Hello World!</h1>");
}

//private null constructor to prevent instantiation?
PHP_METHOD(FCGXRequest, __construct) {}
zend_function_entry fcgx_methods[] = {
	PHP_ME(FCGXRequest, __construct, arginfo_null, ZEND_ACC_PRIVATE)
	{NULL, NULL, NULL}
};

/* ==== End Class Definition ==== */

/* ==== Module initilization ==== */
PHP_MINIT_FUNCTION(fcgx) {
	FCGX_Init();
	
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "-FCGXRequest", fcgx_methods);
	fcgx_entry = zend_register_internal_class(&ce TSRMLS_CC);
	
	return SUCCESS;
}

/* ==== EndModule initilization ==== */

/* ==== Global Gunctions ==== */
int listenfd = -1;

PHP_FUNCTION(FCGX_Listen)
{
	char *sock;
	int   sock_len;
	int   argc = ZEND_NUM_ARGS();
	
	if (zend_parse_parameters(argc TSRMLS_CC, "s", &sock, &sock_len)
			== FAILURE) {
		RETURN_BOOL(0);
	}
	
	//currently cant re-open a socket
	if(listenfd != -1)
		RETURN_BOOL(0);
	
	//make sure its null terminated
	sock[sock_len] = 0;
	
	//printf("Listening on %s\n", sock);

	int sockfd = FCGX_OpenSocket(sock, 1024);
	if(sockfd != -1) {
		listenfd = sockfd;
	}
	
	RETURN_BOOL(sockfd != -1);
}

PHP_FUNCTION(FCGX_Accept)
{
	int i, ret;
	FCGXReq *req = emalloc(sizeof(FCGXReq));
	
	ret = FCGX_InitRequest(req->request, listenfd, 0);
	if(ret != 0) {
		printf("Failed to init request");
		RETURN_NULL();
	}
	
	ret = FCGX_Accept_r(req->request);
	if(ret != 0) {
		printf("Failed on accept");
		RETURN_NULL();
	}
	
	php_stream *_in, *_out, *_err;
	
	MAKE_STD_ZVAL(req->env);
	array_init(req->env);
	for(i = 0; req->request.envp[i] != NULL; i++) {
		char tmp[4096], *key, *val;
		tmp[4095] = 0;
		
		strncpy(tmp, req->request.envp[i], 4095);
		
		key = tmp;
		val = strchr(tmp, '=');
		if(val == NULL)
			val = " ";
		else
			*val = 0;
		val++;
		
		//printf("ENV: %s => %s :: %s\n", key, val, request.envp[i]);
		
		add_assoc_string(req->env, key, val, 1);
	}
	
	_in  = php_stream_from_fcgx(req->request.in,  "r");
	_out = php_stream_from_fcgx(req->request.out, "w");
	_err = php_stream_from_fcgx(req->request.err, "w");
	
	MAKE_STD_ZVAL(req->in);
	MAKE_STD_ZVAL(req->out);
	MAKE_STD_ZVAL(req->err);
	
	php_stream_to_zval(_in,  req->in);
	php_stream_to_zval(_out, req->out);
	php_stream_to_zval(_err, req->err);
	
	array_init(return_value);
	add_assoc_zval(return_value, "in",  req->in);
	add_assoc_zval(return_value, "out", req->out);
	add_assoc_zval(return_value, "err", req->err);
	add_assoc_zval(return_value, "env", req->arr);
	
	object_and_properties_init(return_value, fcgx_entry, Z_ARRVAL_P(return_value));
	
	//RETURN_BOOL(ret == 0)
	//RETURN_ARRAY(arr);
}

