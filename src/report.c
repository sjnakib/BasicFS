#include "vsfs.h"

#include <stdio.h>

static const char *const section_names[VSFS_ISSUE_COUNT] = {
    "Superblock",
    "Inode bitmap",
    "Data bitmap",
    "Duplicate blocks",
    "Bad blocks",
};

size_t report_issue_count(const vsfs_check *check, vsfs_issue_type type)
{
    size_t count = 0;
    size_t index;

    for (index = 0; index < check->issues.count; index++) {
        if (check->issues.items[index].type == type) {
            count++;
        }
    }
    return count;
}

void report_print(const vsfs_check *check)
{
    size_t index;
    size_t total = check->issues.count;
    int type;

    for (type = 0; type < VSFS_ISSUE_COUNT; type++) {
        size_t found = report_issue_count(check, (vsfs_issue_type)type);

        printf("%s\n", section_names[type]);
        if (found == 0) {
            printf("  ok\n");
        } else {
            for (index = 0; index < check->issues.count; index++) {
                const vsfs_issue *issue = &check->issues.items[index];

                if (issue->type == (vsfs_issue_type)type) {
                    printf("  error: %s\n", issue->message);
                }
            }
        }
        printf("\n");
    }

    if (total == 0) {
        printf("No issues found.\n");
    } else {
        printf("%zu issue%s found.\n", total, total == 1 ? "" : "s");
    }
}
