#include <notcurses/notcurses.h>
#include <unistd.h>

int main(void)
{
    struct notcurses_options opts = {};
    struct notcurses *nc = notcurses_core_init(&opts, NULL);
    if (!nc)
        return 1;

    struct ncplane *stdplane = notcurses_stdplane(nc);
    ncplane_putstr(stdplane, "Hello from C and Notcurses!");
    notcurses_render(nc);
    sleep(2);

    notcurses_stop(nc);
    return 0;
}
