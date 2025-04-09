/*
 *  This Confetti example is dedicated to the public domain.
 *  No rights reserved.
 *  http://creativecommons.org/publicdomain/zero/1.0/
 */

// This example reads a Confetti configuration unit from standard input, parses it
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

static void print_directive(conf_directive *dir, int depth)
{
    //
    // Print this directives arguments, separated by a space character.
    //

    const long argument_count = conf_get_argument_count(dir);
    if (argument_count > 0)
    {
        indent(depth);
        for (long i = 0; i < argument_count; i++)
        {
            conf_argument *arg = conf_get_argument(dir, i);
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

    const long subdirective_count = conf_get_directive_count(dir);
    if (subdirective_count > 0)
    {
        puts(" {");
        for (long i = 0; i < subdirective_count; i++)
        {
            conf_directive *subdirective = conf_get_directive(dir, i);
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

    conf_error err = {0};
    conf_unit *unit = conf_parse(input, NULL, &err);
    if (unit == NULL)
    {
        printf("error: %s\n", err.description);
        return 1;
    }

    //
    // (3) Pretty print the Confetti.
    //

    conf_directive *root = conf_get_root(unit);
    for (long i = 0; i < conf_get_directive_count(root); i++)
    {
        conf_directive *directive = conf_get_directive(root, i);
        print_directive(directive, 0);
    }

    //
    // (4) Cleanup after ourselves.
    //
    
    conf_free(unit);
    free(input);
    return 0;
}
