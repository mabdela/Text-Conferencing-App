//
// PrintHelper headers file
 
#pragma once
#ifndef PRINTHELPERS_H_
#define PRINTHELPERS_H_

#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Helper function for printing the last raised errno to stderr
 */
void printLastError(char *format);

/**
 * @brief Parses the string with the given delimiters, returning a reference to an
 * array containing the tokens and the size in the count parameter
 *
 * @params string String to tokenize
 * @params delimiter Delimiter string
 * @params count Return variable for the number of tokens parsed
 * @returns Array of parsed tokens
 */
char **parseTokens(char *string, char *delimiter, int *count);

#endif
