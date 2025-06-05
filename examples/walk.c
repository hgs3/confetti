/*
 *  This Confetti example is dedicated to the public domain.
 *  No rights reserved.
 *  http://creativecommons.org/publicdomain/zero/1.0/
 */

// This example reads a Confetti configuration unit from standard input and uses the
// directive walker API to visit each directive, pretty printing them to
// standard output.

// Using the directive walker is more memory efficient than using the parse() API as
// the latter builds an in-memory representation of the configuration unit whereas the
// former does not.

#include "confetti.h"
#include <stdio.h>
#include <stdlib.h>

char *readstdin(void);

static void indent(int depth)
{
    for (int i = 0; i < depth; i++)
    {
        printf("    "); // four space indention (change as desired)
    }
}

static int step(void *user_data, conf_element type, int argc, const conf_argument *argv, const conf_comment *comment)
{
    int *depth = user_data; // The subdirective depth is used to indent the output when pretty printing.
    switch (type)
    {
    case CONF_DIRECTIVE:
        indent(*depth);
        for (long i = 0; i < argc; i++)
        {
            printf("%s", argv[i].value);
            if (i < (argc - 1))
            {
                printf(" ");
            }
        }
        putchar('\n');
        break;

    case CONF_BLOCK_ENTER:
        indent(*depth);
        puts("{");
        (*depth) += 1;
        break;

    case CONF_BLOCK_LEAVE:
        (*depth) -= 1;
        indent(*depth);
        puts("}");
        break;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    //
    // (1) Read Confetti from standard input.
    //

    char *input = readstdin();

    //
    // (2) Walk the Confetti source text.
    //

    int depth = 0; // Track the subdirective depth to know how much to indent when pretty printing.
    conf_options options = { .user_data = &depth };
    conf_error error = {0};
    conf_walk(input, &options, &error, step);
    if (error.code != CONF_NO_ERROR)
    {
        printf("error: %s\n", error.description);
        free(input);
        return 1;
    }

    //
    // (3) Cleanup after ourselves.
    //
    
    free(input);
    return 0;
}
