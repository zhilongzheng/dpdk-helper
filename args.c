/*
* Created by Zhilong Zheng
*/

#include <stddef.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <rte_compat.h>

static int
parse_portmask(const char *portmask)
{
    unsigned long pm;
    char *end = NULL;

    /* parse hexadecimal string */
    pm = strtoul(portmask, &end, 16);
    if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
        return -1;

    if (pm == 0) {
        rte_exit(EXIT_FAILURE, "failure when parsing portmask\n");
    }

    return pm;
}


int get_portmask(int argc, char **argv)
{
    int opt;
    int option_index;
    char **argvopt;
    char *prgname = argv[0];
    int portmask = 0;
    static struct option lgopts[] = {
        {NULL, 0, 0, 0}
    };

    argvopt = argv;

    while ((opt = getopt_long(argc, argvopt, "p:",
                              lgopts, &option_index)) != EOF) {
        switch (opt) {
            /* portmask */
            case 'p':
                portmask = parse_portmask(optarg);
                if (portmask == 0) {
                    printf("invalid portmask\n");
                    return -1;
                }
                break;
            default:
                return -1;
        }
    }
    if (optind <= 1) {
        rte_exit(EXIT_FAILURE, "failure when getting portmask\n");
    }

    argv[optind-1] = prgname;
    optind = 0; /* reset getopt lib */
    return portmask;
}
