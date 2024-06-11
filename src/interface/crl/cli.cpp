#include <cstddef>
#include <cstdio>
#include <cstring>

#include "crossline.h"

#include "interface/crl/cli.hpp"

CommandReadLine::CommandReadLine()
{
}

CommandReadLine::~CommandReadLine()
{
}

bool CommandReadLine::mainloop()
{
    char buf[256];

    bool quit = crossline_readline("mirabel > ", buf, sizeof(buf)) == NULL;
    if (quit || strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) {
        printf("DOEN\n");
        return true;
    }

    printf("echo: \"%s\"\n", buf);

    return false;
}
