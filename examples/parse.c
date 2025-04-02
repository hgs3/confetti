/*
 *  This Confetti example is dedicated to the public domain.
 *  No rights reserved.
 *  http://creativecommons.org/publicdomain/zero/1.0/
 */

// This example reads a Confetti document from standard input, parses it
// into an in-memory representation, then pretty prints it to standard output.

// Most configuration and data-interchange parsers build an in-memory representation
// so this API will feel more familair. For a more memory efficient approach, see
// the directive walker example.

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

static void print_directive(conf_dir *dir, int depth)
{
    //
    // Print this directives arguments, separated by a space character.
    //

    const long argument_count = conf_getnarg(dir);
    if (argument_count > 0)
    {
        indent(depth);
        for (long i = 0; i < argument_count; i++)
        {
            conf_arg *arg = conf_getarg(dir, i);
            printf("%s", arg->value);
            if (i < (argument_count - 1))
            {
                printf(" ");
            }
        }
    }

    //
    // Recursively print this directives subdirectives.
    //

    const long subdirective_count = conf_getnsubdir(dir);
    if (subdirective_count > 0)
    {
        puts(" {");
        for (long i = 0; i < subdirective_count; i++)
        {
            conf_dir *subdirective = conf_getsubdir(dir, i);
            print_directive(subdirective, depth + 1);
        }

        indent(depth);
        putchar('}');
    }

    puts(""); // Print each directive on its own line.
}

int main(int argc, char *argv[])
{
    //
    // (1) Read Confetti from standard input.
    //

    char *input = readstdin();

    //
    // (2) Parse the input as Confetti, checking for errors.
    //

    conf_err err = {0};
    conf_doc *dir = conf_parse(input, NULL, &err);
    if (dir == NULL)
    {
        printf("error: %s\n", err.description);
        return 1;
    }

    //
    // (3) Pretty print the Confetti.
    //

    for (long i = 0; i < conf_getndir(dir); i++)
    {
        conf_dir *subdir = conf_getdir(dir, i);
        print_directive(subdir, 0);
    }

    //
    // (4) Cleanup after ourselves.
    //
    
    conf_free(dir);
    free(input);
    return 0;
}
