//
// PrintHelper implementation
 

#include "printHelpers.h"

/**
 * @brief Helper function for printing the last raised errno to stderr
 */
void printLastError(char *format) {
    fprintf(stderr, format, strerror(errno));
}

/**
 * @brief Parses the string with the given delimiters, returning a reference to an
 * array containing the tokens and the size in the count parameter
 *
 * @params string String to tokenize
 * @params delimiter Delimiter string
 * @params count Return variable for the number of tokens parsed
 * @returns Array of parsed tokens
 */
char **parseTokens(char *string, char *delimiter, int *count)
{
	// Start with initial buffer size of 8
	int n = 0, currMax = 8, i = 0;
	char *token = NULL, *savePtr = NULL;
	char **tempBuffer = (char **)calloc(currMax, sizeof(char *));

	token = strtok_r(string, delimiter, &savePtr);

	// Continously extract tokens until there's none remaining
	do
	{
		if (token != NULL)
		{
			if (n < currMax)
			{
				tempBuffer[n++] = token;
			}
			else
			{
				// Exceed buffer size, double the buffer
				currMax *= 2;
				// Allocate a new pointer buffer
				char **secondary = (char **)calloc(currMax, sizeof(char *));
				// Element-wise copy over
				for (i = 0; i < n; i++)
				{
					secondary[i] = tempBuffer[i];
				}
				// Add the current element
				secondary[n++] = token;
				// Free the old buffer and swap the pointers
				free(tempBuffer);
				tempBuffer = secondary;
			}
			token = NULL;
		}

		token = strtok_r(NULL, delimiter, &savePtr);
	} while (token != NULL);

	// Resize the return element to the correct memory size
	if (n < currMax)
	{
		// Allocate a properly sized array
		char **temporary = (char **)calloc(n, sizeof(char *));
		// Element-wise copy over
		for (i = 0; i < n; i++)
		{
			temporary[i] = tempBuffer[i];
		}
		// Free the old buffer and swap pointers
		free(tempBuffer);
		tempBuffer = temporary;
	}

	// Set the return count
	*count = n;
	return tempBuffer;
}
