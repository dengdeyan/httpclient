#ifndef STRINGX_H_INCLUDE
#define STRINGX_H_INCLUDE

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
char* str_new(int len);
void str_free(char* str);
/*
	Gets the offset of one string in another string
*/
int str_index_of(const char *a, char *b);
/*
	Checks if one string contains another string
*/
int str_contains(const char *haystack, const char *needle);
/*
	Removes last character from string
*/
char* trim_end(char *string, char to_trim);
/*
	Concecates two strings, a wrapper for strcat from string.h, handles the resizing and copying
*/
char* str_cat(char *a, char *b);
/*
	Converts an integer value to its hex character
*/
char to_hex(char code);
/*
	URL encodes a string
*/
char *urlencode(char *str);
/*
	Replacement for the string.h strndup, fixes a bug
*/
char *str_ndup (const char *str, size_t max);
/*
	Replacement for the string.h strdup, fixes a bug
*/
char *str_dup(const char *src);
/*
 	Search and replace a string with another string , in a string
*/
char *str_replace(char *search , char *replace , char *subject);
/*
	Get's all characters until '*until' has been found
*/
char* get_until(char *haystack, char *until);
/*
	Decodes a Base64 string
*/
char* str_base64_decode(char *b64src);
/*
	Encodes a string with Base64
*/
char* str_base64_encode(char *clrstr);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // STRINGX_H_INCLUDE
