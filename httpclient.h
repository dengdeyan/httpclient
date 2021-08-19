#ifndef HTTPCLIENT_H_INCLUDE
#define HTTPCLIENT_H_INCLUDE


#include "urlparser.h"
/*
	Represents an HTTP html response
*/
struct http_response
{
	struct parsed_url *request_uri;
	char *body;
	char *status_code;
	int status_code_int;
	char *status_text;
	char *request_headers;
	char *response_headers;
};

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*
	Prototype functions
*/
struct http_response* http_req(char *http_headers, struct parsed_url *purl);
struct http_response* http_get(char *url, char *custom_headers);
struct http_response* http_head(char *url, char *custom_headers);
struct http_response* http_post(char *url, char *custom_headers, char *post_data);
void http_response_free(struct http_response *hresp);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // HTTPCLIENT_H_INCLUDE
