#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
    if (str == NULL) return;
    
    int length = strlen(str);
    int left = 0;
    int right = length - 1;
    
    while (left < right)
    {
        // Обмен символов
        char temp = str[left];
        str[left] = str[right];
        str[right] = temp;
        
        left++;
        right--;
    }
}
