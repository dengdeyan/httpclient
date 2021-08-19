#include "httpclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <stdio.h>
#elif _LINUX
	#include <sys/socket.h>
#elif __FreeBSD__ || __ANDROID__ || ANDROID
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <arpa/inet.h>
#else
	#error Platform not suppoted.
#endif

#include <errno.h>
#include "stringx.h"

#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#define TAG "xxx" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型w
#else
#define LOGD(...) printf(__VA_ARGS__)
#endif

/*
	Handles redirect if needed for get requests
*/
struct http_response* handle_redirect_get(struct http_response* hresp, char* custom_headers)
{
	if(hresp->status_code_int > 300 && hresp->status_code_int < 399)
	{
		char *token = strtok(hresp->response_headers, "\r\n");
		while(token != NULL)
		{
			if(str_contains(token, "Location:"))
			{
				/* Extract url */
				char *location = str_replace("Location: ", "", token);
				return http_get(location, custom_headers);
			}
			token = strtok(NULL, "\r\n");
		}
	}
	else
	{
		/* We're not dealing with a redirect, just return the same structure */
		return hresp;
	}
}

/*
	Handles redirect if needed for head requests
*/
struct http_response* handle_redirect_head(struct http_response* hresp, char* custom_headers)
{
	if(hresp->status_code_int > 300 && hresp->status_code_int < 399)
	{
		char *token = strtok(hresp->response_headers, "\r\n");
		while(token != NULL)
		{
			if(str_contains(token, "Location:"))
			{
				/* Extract url */
				char *location = str_replace("Location: ", "", token);
				return http_head(location, custom_headers);
			}
			token = strtok(NULL, "\r\n");
		}
	}
	else
	{
		/* We're not dealing with a redirect, just return the same structure */
		return hresp;
	}
}

/*
	Handles redirect if needed for post requests
*/
struct http_response* handle_redirect_post(struct http_response* hresp, char* custom_headers, char *post_data)
{
	if(hresp->status_code_int > 300 && hresp->status_code_int < 399)
	{
		char *token = strtok(hresp->response_headers, "\r\n");
		while(token != NULL)
		{
			if(str_contains(token, "Location:"))
			{
				/* Extract url */
				char *location = str_replace("Location: ", "", token);
				return http_post(location, custom_headers, post_data);
			}
			token = strtok(NULL, "\r\n");
		}
	}
	else
	{
		/* We're not dealing with a redirect, just return the same structure */
		return hresp;
	}
}

/*
	Makes a HTTP request and returns the response
*/
struct http_response* http_req(char *http_headers, struct parsed_url *purl)
{
	/* Declare variable */
	int sock;
	int tmpres;
	char buf[BUFSIZ+1];
	struct sockaddr_in *remote;

	/* Allocate memeory for htmlcontent */
	struct http_response *hresp = (struct http_response*)malloc(sizeof(struct http_response));
	if(hresp == NULL)
	{
		LOGD("Unable to allocate memory for htmlcontent.\n");
		free(http_headers);
		return NULL;
	}
	hresp->body = NULL;
	hresp->request_headers = NULL;
	hresp->response_headers = NULL;
	hresp->status_code = NULL;
	hresp->status_text = NULL;

	/* Create TCP socket */
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
	    LOGD("Can't create TCP socket\n");
		free(http_headers);
	    free(hresp);
		return NULL;
	}

	/* Set remote->sin_addr.s_addr */
	remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	remote->sin_family = AF_INET;
  	tmpres = inet_pton(AF_INET, purl->ip, (void *)(&(remote->sin_addr.s_addr)));
  	if( tmpres < 0)
  	{
    	LOGD("Can't set remote->sin_addr.s_addr\n");
		free(http_headers);
    	free(remote);
    	free(hresp);
    	return NULL;
  	}
	else if(tmpres == 0)
  	{
		LOGD("Not a valid IP\n");
		free(http_headers);
        free(remote);
        free(hresp);
    	return NULL;
  	}
	remote->sin_port = htons(atoi(purl->port));

	/* Connect */
	if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0)
	{
	    LOGD("Could not connect\n");
		free(http_headers);
        free(remote);
        free(hresp);
		return NULL;
	}
	/* Send headers to server */
	int sent = 0;
	while(sent < strlen(http_headers))
	{
	    tmpres = send(sock, http_headers+sent, strlen(http_headers)-sent, 0);
		if(tmpres == -1)
		{
			LOGD("Can't send headers\n");
			free(http_headers);
            free(remote);
            free(hresp);
			return NULL;
		}
		sent += tmpres;
	 }

	/* Recieve into response*/
	char *response = str_new(16);
	char BUF[BUFSIZ];
	ssize_t recived_len = 0;
	while((recived_len = recv(sock, BUF, BUFSIZ-1, 0)) > 0)
	{
        BUF[recived_len] = '\0';
		response = (char*)realloc(response, strlen(response) + strlen(BUF) + 1);
		sprintf(response, "%s%s", response, BUF);
	}
	if (recived_len < 0)
    {
		free(http_headers);
        free(remote);
        free(hresp);
        free(response);
		#ifdef _WIN32
			closesocket(sock);
		#else
			close(sock);
		#endif
        LOGD("Unabel to recieve\n");
		return NULL;
    }

	/* Reallocate response */
	response = (char*)realloc(response, strlen(response) + 1);

	/* Close socket */
	#ifdef _WIN32
		closesocket(sock);
	#else
		close(sock);
	#endif

	/* Parse status code and text */
	char *tmp_status_line = get_until(response, "\r\n");
	char* status_line = str_replace("HTTP/1.1 ", "", tmp_status_line);
	char *tmp_status_code = str_ndup(status_line, 4);
	char* status_code = str_replace(" ", "", tmp_status_code);
	char *tmp_status_text = str_replace(status_code, "", status_line);
	char* status_text = str_replace(" ", "", tmp_status_text);
	hresp->status_code = status_code;
	hresp->status_code_int = atoi(status_code);
	hresp->status_text = status_text;
	free(status_line);
	free(tmp_status_line);
	free(tmp_status_code);

	/* Parse response headers */
	char *headers = get_until(response, "\r\n\r\n");
	hresp->response_headers = headers;

	/* Assign request headers */
	hresp->request_headers = http_headers;

	/* Assign request url */
	hresp->request_uri = purl;

	/* Parse body */
	char *body = strstr(response, "\r\n\r\n");
	body = str_replace("\r\n\r\n", "", body);
	hresp->body = body;
	free(response);
	/* Return response */
	return hresp;
}

/*
	Makes a HTTP GET request to the given url
*/
struct http_response* http_get(char *url, char *custom_headers)
{
	/* Parse url */
	struct parsed_url *purl = parse_url(url);
	if(purl == NULL || purl->ip==NULL)
	{
		printf("Unable to parse url");
		return NULL;
	}

	/* Declare variable */
	char *http_headers = str_new(1024);
	/* Build query/headers */
	if(purl->path != NULL)
	{
		if(purl->query != NULL)
		{
			sprintf(http_headers, "GET /%s?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->path, purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->path, purl->host);
		}
	}
	else
	{
		if(purl->query != NULL)
		{
			sprintf(http_headers, "GET /?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "GET / HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->host);
		}
	}

	/* Handle authorisation if needed */
	if(purl->username != NULL)
	{
		/* Format username:password pair */
		char *upwd = str_new(1024);
		sprintf(upwd, "%s:%s", purl->username, purl->password);
		upwd = (char*)realloc(upwd, strlen(upwd) + 1);

		/* Base64 encode */
		char *base64 = str_base64_encode(upwd);

		/* Form header */
		char *auth_header = str_new(1024);
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		auth_header = (char*)realloc(auth_header, strlen(auth_header) + 1);

		/* Add to header */
		http_headers = (char*)realloc(http_headers, strlen(http_headers) + strlen(auth_header) + 2);
		sprintf(http_headers, "%s%s", http_headers, auth_header);
		free(upwd);
		free(base64);
	}

	/* Add custom headers, and close */
	if(custom_headers != NULL)
	{
		sprintf(http_headers, "%s%s\r\n", http_headers, custom_headers);
	}
	else
	{
		sprintf(http_headers, "%s\r\n", http_headers);
	}
	http_headers = (char*)realloc(http_headers, strlen(http_headers) + 1);

	/* Make request and return response */
	struct http_response *hresp = http_req(http_headers, purl);

    if(hresp==NULL) return NULL;
	/* Handle redirect */
	return handle_redirect_get(hresp, custom_headers);
}

/*
	Makes a HTTP POST request to the given url
*/
struct http_response* http_post(char *url, char *custom_headers, char *post_data)
{
	/* Parse url */
	struct parsed_url *purl = parse_url(url);
	if(purl == NULL || purl->ip==NULL)
	{
		LOGD("Unable to parse url\n");
		return NULL;
	}
	/* Declare variable */
	char *http_headers = str_new(1024);
	/* Build query/headers */
	if(purl->path != NULL)
	{
		if(purl->query != NULL)
		{
			sprintf(http_headers, "POST /%s?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\nContent-Length:%zu\r\nContent-Type:application/x-www-form-urlencoded\r\n", purl->path, purl->query, purl->host, strlen(post_data));
		}
		else
		{
			sprintf(http_headers, "POST /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\nContent-Length:%zu\r\nContent-Type:application/x-www-form-urlencoded\r\n", purl->path, purl->host, strlen(post_data));
		}
	}
	else
	{
		if(purl->query != NULL)
		{
			sprintf(http_headers, "POST /?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\nContent-Length:%zu\r\nContent-Type:application/x-www-form-urlencoded\r\n", purl->query, purl->host, strlen(post_data));
		}
		else
		{
			sprintf(http_headers, "POST / HTTP/1.1\r\nHost:%s\r\nConnection:close\r\nContent-Length:%zu\r\nContent-Type:application/x-www-form-urlencoded\r\n", purl->host, strlen(post_data));
		}
	}

	/* Handle authorisation if needed */
	if(purl->username != NULL)
	{
		/* Format username:password pair */
		char *upwd = str_new(1024);
		sprintf(upwd, "%s:%s", purl->username, purl->password);
		upwd = (char*)realloc(upwd, strlen(upwd) + 1);

		/* Base64 encode */
		char *base64 = str_base64_encode(upwd);
		/* Form header */
		char *auth_header = str_new(1024);
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		auth_header = (char*)realloc(auth_header, strlen(auth_header) + 1);

		/* Add to header */
		http_headers = (char*)realloc(http_headers, strlen(http_headers) + strlen(auth_header) + 2);
		sprintf(http_headers, "%s%s", http_headers, auth_header);
		free(upwd);
		free(base64);
		free(auth_header);
	}

	if(custom_headers != NULL)
	{
		sprintf(http_headers, "%s%s\r\n", http_headers, custom_headers);
		sprintf(http_headers, "%s\r\n%s", http_headers, post_data);
	}
	else
	{
		sprintf(http_headers, "%s\r\n%s", http_headers, post_data);
	}
	http_headers = (char*)realloc(http_headers, strlen(http_headers) + 1);

	/* Make request and return response */
	struct http_response *hresp = http_req(http_headers, purl);
    if(hresp==NULL) return NULL;
	/* Handle redirect */
	return handle_redirect_post(hresp, custom_headers, post_data);
}

/*
	Makes a HTTP HEAD request to the given url
*/
struct http_response* http_head(char *url, char *custom_headers)
{
	/* Parse url */
	struct parsed_url *purl = parse_url(url);
	if(purl == NULL || purl->ip==NULL)
	{
		printf("Unable to parse url");
		return NULL;
	}

	/* Declare variable */
	char *http_headers = str_new(1024);

	/* Build query/headers */
	if(purl->path != NULL)
	{
		if(purl->query != NULL)
		{
			sprintf(http_headers, "HEAD /%s?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->path, purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "HEAD /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->path, purl->host);
		}
	}
	else
	{
		if(purl->query != NULL)
		{
			sprintf(http_headers, "HEAD/?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "HEAD / HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->host);
		}
	}

	/* Handle authorisation if needed */
	if(purl->username != NULL)
	{
		/* Format username:password pair */
		char *upwd = str_new(1024);
		sprintf(upwd, "%s:%s", purl->username, purl->password);
		upwd = (char*)realloc(upwd, strlen(upwd) + 1);

		/* Base64 encode */
		char *base64 = str_base64_encode(upwd);

		/* Form header */
		char *auth_header = str_new(1024);
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		auth_header = (char*)realloc(auth_header, strlen(auth_header) + 1);

		/* Add to header */
		http_headers = (char*)realloc(http_headers, strlen(http_headers) + strlen(auth_header) + 2);
		sprintf(http_headers, "%s%s", http_headers, auth_header);
	}

	if(custom_headers != NULL)
	{
		sprintf(http_headers, "%s%s\r\n", http_headers, custom_headers);
	}
	else
	{
		sprintf(http_headers, "%s\r\n", http_headers);
	}
	http_headers = (char*)realloc(http_headers, strlen(http_headers) + 1);

	/* Make request and return response */
	struct http_response *hresp = http_req(http_headers, purl);

	/* Handle redirect */
	return handle_redirect_head(hresp, custom_headers);
}

/*
	Do HTTP OPTIONs requests
*/
struct http_response* http_options(char *url)
{
	/* Parse url */
	struct parsed_url *purl = parse_url(url);
	if(purl == NULL || purl->ip==NULL)
	{
		printf("Unable to parse url");
		return NULL;
	}

	/* Declare variable */
	char *http_headers = str_new(1024);

	/* Build query/headers */
	if(purl->path != NULL)
	{
		if(purl->query != NULL)
		{
			sprintf(http_headers, "OPTIONS /%s?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->path, purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "OPTIONS /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->path, purl->host);
		}
	}
	else
	{
		if(purl->query != NULL)
		{
			sprintf(http_headers, "OPTIONS/?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "OPTIONS / HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->host);
		}
	}

	/* Handle authorisation if needed */
	if(purl->username != NULL)
	{
		/* Format username:password pair */
		char *upwd = str_new(1024);
		sprintf(upwd, "%s:%s", purl->username, purl->password);
		upwd = (char*)realloc(upwd, strlen(upwd) + 1);

		/* Base64 encode */
		char *base64 = str_base64_encode(upwd);

		/* Form header */
		char *auth_header = str_new(1024);
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		auth_header = (char*)realloc(auth_header, strlen(auth_header) + 1);

		/* Add to header */
		http_headers = (char*)realloc(http_headers, strlen(http_headers) + strlen(auth_header) + 2);
		sprintf(http_headers, "%s%s", http_headers, auth_header);
	}

	/* Build headers */
	sprintf(http_headers, "%s\r\n", http_headers);
	http_headers = (char*)realloc(http_headers, strlen(http_headers) + 1);

	/* Make request and return response */
	struct http_response *hresp = http_req(http_headers, purl);

	/* Handle redirect */
	return hresp;
}

/*
	Free memory of http_response
*/
void http_response_free(struct http_response *hresp)
{
	if(hresp != NULL)
	{
		if(hresp->request_uri != NULL) parsed_url_free(hresp->request_uri);
		if(hresp->body != NULL) free(hresp->body);
		if(hresp->status_code != NULL) free(hresp->status_code);
		if(hresp->status_text != NULL) free(hresp->status_text);
		if(hresp->request_headers != NULL) free(hresp->request_headers);
		if(hresp->response_headers != NULL) free(hresp->response_headers);
		free(hresp);
	}
}
