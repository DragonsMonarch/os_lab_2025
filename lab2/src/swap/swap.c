#include "swap.h"

void Swap(char *left, char *right)
{
	char mid = *right;
	*right = *left;
	*left = mid;
}
