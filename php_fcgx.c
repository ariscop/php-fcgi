/* ==== PHP Module definitions ==== */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_fcgx.h"

static zend_function_entry fcgx_functions[] = {
    PHP_FE(FCGX_Init, NULL)
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

/* ==== Function definitions ==== */

#include <fcgiapp.h>

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

/**
 * API:
 * FCGX_Init("address"): initilize and listen on a port/socket
 * FCGX_Accept: Accept a request
 * 
 * Two options:
 * return an object from FCGX_Accept:
 * ->in
 * ->out
 * ->err : streams for in/out/err, destroyed when finished
 * ->env : environment from fcgi
 * ->flush(?) : flushes output streams
 * ->finish : finishes the request
 * 
 * Second option:
 * same as above but keep functions seperate from the object
 * 
 */


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

struct php_fcgx_private {
	FCGX_Request request;
	zval   *in;
	zval   *out;
	zval   *err;
	int running;
};

struct php_fcgx_private priv;

PHP_MINIT_FUNCTION(fcgx) {
	FCGX_Init();
}

PHP_FUNCTION(FCGX_Init)
{
	char *sock;
	int   sock_len;
	int argc = ZEND_NUM_ARGS();
	
	if (zend_parse_parameters(argc TSRMLS_CC, "s", &sock, &sock_len)
			== FAILURE) {
		RETURN_BOOL(0);
	}
	
	//make sure its null terminated
	sock[sock_len] = 0;
	
	printf("Listening on %s\n", sock);

	int sockfd = FCGX_OpenSocket(sock, 1024);
	//FCGX_Init();
	int ret = FCGX_InitRequest(&priv.request, sockfd, 0);
	RETURN_BOOL(ret == 0);
}

PHP_FUNCTION(FCGX_Accept)
{
	int i, ret;
	
	ret = FCGX_Accept_r(&priv.request);
	if(ret != 0)
		RETURN_NULL();
	
	php_stream *_in, *_out, *_err;
	zval *in, *out, *err, *arr;
	
	MAKE_STD_ZVAL(arr);
	array_init(arr);
	for(i = 0; priv.request.envp[i] != NULL; i++) {
		char tmp[4096], *key, *val;
		tmp[4095] = 0;
		
		strncpy(tmp, priv.request.envp[i], 4095);
		
		key = tmp;
		val = strchr(tmp, '=');
		if(val == NULL)
			val = " ";
		else
			*val = 0;
		val++;
		
		//printf("ENV: %s => %s :: %s\n", key, val, request.envp[i]);
		
		add_assoc_string(arr, key, val, 1);
	}
	
	_in  = php_stream_from_fcgx(priv.request.in,  "r");
	_out = php_stream_from_fcgx(priv.request.out, "w");
	_err = php_stream_from_fcgx(priv.request.err, "w");
	
	MAKE_STD_ZVAL(in);
	MAKE_STD_ZVAL(out);
	MAKE_STD_ZVAL(err);
	
	php_stream_to_zval(_in,  in);
	php_stream_to_zval(_out, out);
	php_stream_to_zval(_err, err);
	
	
	array_init(return_value);
	add_assoc_zval(return_value, "in",  in);
	add_assoc_zval(return_value, "out", out);
	add_assoc_zval(return_value, "err", err);
	add_assoc_zval(return_value, "env", arr);
	
	//RETURN_BOOL(ret == 0)
	//RETURN_ARRAY(arr);
}

PHP_FUNCTION(FCGX_Finish)
{
	FCGX_Finish_r(&priv.request);
}

PHP_FUNCTION(FCGX_Hello_World)
{
	FCGX_FPrintF(priv.request.out, "HTTP 200 Ok\r\nContent-type: text/html\r\n\r\n <h1>Hello World!</h1>");
}

