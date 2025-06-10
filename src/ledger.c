#include <notcurses/notcurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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
    snprintf(entries[entry_count].hostname, sizeof(entries[entry_count].hostname), "host%d.local", entry_count);
    snprintf(entries[entry_count].last_accessed, sizeof(entries[entry_count].last_accessed), "2025-06-10 21:37");
    snprintf(entries[entry_count].name, sizeof(entries[entry_count].name), "%s", name);
    snprintf(entries[entry_count].description, sizeof(entries[entry_count].description), "This is a dummy description for %s", name);
    entry_count++;
}

void render_entries(struct notcurses *nc, struct ncplane *plane, int selected)
{
    ncplane_erase(plane);

    for (int i = 0; i < entry_count; ++i)
    {
        if (i == selected)
        {
            ncplane_set_fg_rgb(plane, 0x000000);
            ncplane_set_bg_rgb(plane, 0x00ff00); // highlighted row
        }
        else
        {
            ncplane_set_fg_rgb(plane, 0xffffff);
            ncplane_set_bg_rgb(plane, 0x000000);
        }

        char line[384];
        snprintf(line, sizeof(line), "[%s] %s - %s (%s)\n",
                 entries[i].last_accessed,
                 entries[i].hostname,
                 entries[i].name,
                 entries[i].description);

        ncplane_putstr(plane, line);
        ncplane_cursor_move_yx(plane, i + 1, 0);
    }

    notcurses_render(nc);
}

int main(void)
{
    setlocale(LC_ALL, "");

    struct notcurses_options opts = {
        .flags = NCOPTION_INHIBIT_SETLOCALE // already calling setlocale manually
    };

    struct notcurses *nc = notcurses_core_init(&opts, NULL);
    if (!nc)
    {
        fprintf(stderr, "Failed to init notcurses.\n");
        return EXIT_FAILURE;
    }

    struct ncplane *stdplane = notcurses_stdplane(nc);

    struct ncplane_options nopts = {
        .y = 1,
        .x = 2,
        .rows = 60,
        .cols = 384,
        .userptr = NULL,
        .name = "list",
        .resizecb = ncplane_resize_realign,
        .flags = 0,
        .margin_b = 0,
        .margin_r = 0,
    };
    struct ncplane *list_plane = ncplane_create(stdplane, &nopts);

    char32_t ch = 0;
    int selected = 0;

    add_dummy_entry("entry_0");
    add_dummy_entry("entry_1");
    add_dummy_entry("entry_2");

    struct ncinput ni;

    while (ch != 'q')
    {
        render_entries(nc, list_plane, selected);

        int r = notcurses_get_blocking(nc, &ni);
        if (r <= 0)
            continue;

        ch = ni.id;

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

    notcurses_stop(nc);
    return EXIT_SUCCESS;
}
