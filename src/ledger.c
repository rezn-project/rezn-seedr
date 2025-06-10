#include <notcurses/direct.h>
#include <unistd.h>

int main(void)
{
    struct ncdirect *nc = ncdirect_init(NULL, stdout, NCDIRECT_OPTION_INHIBIT_CBREAK);
    if (!nc)
        return 1;

    int ch = 0;
    while (ch != 'q')
    {
        ncdirect_clear(nc);
        ncdirect_cursor_move_yx(nc, 1, 1);
        ncdirect_putstr(nc, 0, "Press 'q' to quit.");

        ncdirect_cursor_move_yx(nc, 2, 1);

        char buf[256];
        snprintf(buf, sizeof(buf), "Last key: %c", ch);
        ncdirect_putstr(nc, 0, buf);

        fflush(stdout);

        ch = ncdirect_get_blocking(nc, NULL);
    }

    ncdirect_stop(nc);
    return 0;
}
