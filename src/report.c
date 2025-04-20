#include "vsfs.h"

#include <stdio.h>

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

    if (check->issues.count == 0) {
        printf("No issues found.\n");
        return;
    }

    for (index = 0; index < check->issues.count; index++) {
        printf("error: %s\n", check->issues.items[index].message);
    }
    printf("%zu issue%s found.\n", check->issues.count,
           check->issues.count == 1 ? "" : "s");
}
