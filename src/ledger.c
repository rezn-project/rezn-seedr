#include <notcurses/direct.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <locale.h>
#include <uchar.h>

#define MAX_ENTRIES 128
#define DESC_LEN 128

typedef struct
{
    char hostname[64];
    char last_accessed[32];
    char name[32];
    char description[DESC_LEN];
} Entry;

Entry entries[MAX_ENTRIES];
int entry_count = 0;

void add_dummy_entry(const char *name)
{
    if (entry_count >= MAX_ENTRIES)
        return;
    snprintf(entries[entry_count].hostname, 64, "host%d.local", entry_count);
    snprintf(entries[entry_count].last_accessed, 32, "2025-06-10 21:37");
    snprintf(entries[entry_count].name, 64, "%s", name);
    snprintf(entries[entry_count].description, DESC_LEN, "This is a dummy description for %s", name);
    entry_count++;
}

void render_entries(struct ncdirect *nc, int selected)
{
    ncdirect_clear(nc);
    for (int i = 0; i < entry_count; ++i)
    {
        if (i == selected)
        {
            ncdirect_set_fg_rgb(nc, 0x000000);
            ncdirect_set_bg_rgb(nc, 0x00ff00);
        }
        else
        {
            ncdirect_set_fg_rgb(nc, 0xdddddd);
            ncdirect_set_bg_rgb(nc, 0x000000);
        }

        ncdirect_cursor_move_yx(nc, i + 1, 2);
        char line[384];
        snprintf(line, sizeof(line), "[%s] %s - %s (%s)",
                 entries[i].last_accessed,
                 entries[i].hostname,
                 entries[i].name,
                 entries[i].description);
        ncdirect_putstr(nc, 0, line);
    }
    fflush(stdout);
}

int main(void)
{
    setlocale(LC_ALL, "");

    struct ncdirect *nc = ncdirect_init(NULL, stdout, NCDIRECT_OPTION_INHIBIT_CBREAK);
    if (!nc)
        return 1;

    // ncdirect_set_fg_rgb(nc, 0x000000);
    // ncdirect_set_bg_rgb(nc, 0x00ff00);

    char32_t ch = 0;
    int selected = 0;

    add_dummy_entry("entry_0");
    add_dummy_entry("entry_1");
    add_dummy_entry("entry_2");

    while (ch != 'q')
    {
        render_entries(nc, selected);

        ch = ncdirect_get_blocking(nc, NULL);

        switch (ch)
        {
        case 'j':
            if (selected < entry_count - 1)
                selected++;
            break;
        case 'k':
            if (selected > 0)
                selected--;
            break;
        case 'a':
            add_dummy_entry("new_entry");
            break;
        }
    }

    ncdirect_stop(nc);
    return 0;
}
