#include "revert_string.h"
#include <stdlib.h>
#include <string.h>
void RevertString(char *str)
{
	// your code here
	char* p = (char *)malloc(strlen(str));
	for (int i =0; i < strlen(str);i++){
		p[i] = str[strlen(str) - i - 1];
	}
	for (int i =0; i < strlen(str);i++){
		str[i] = p[i];
	}
	free(p)
}

