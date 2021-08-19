#ifndef URLPARSER_H_INCLUDE
#define URLPARSER_H_INCLUDE

/*
	Represents an url
*/
struct parsed_url
{
	char *uri;					/* mandatory */
    char *scheme;               /* mandatory */
    char *host;                 /* mandatory */
	char *ip; 					/* mandatory */
    char *port;                 /* optional */
    char *path;                 /* optional */
    char *query;                /* optional */
    char *fragment;             /* optional */
    char *username;             /* optional */
    char *password;             /* optional */
};

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

struct parsed_url *parse_url(const char *url);
void parsed_url_free(struct parsed_url *purl);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // URLPARSER_H_INCLUDE
