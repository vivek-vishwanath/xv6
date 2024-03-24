#include <fcntl.h>
#include "types.h"
#include "user.h"
#include "stat.h"

#define SMALLNBLOCKS 10
#define NSMALLFILES 4
#define NBIGFILES 1
#define BIGNBLOCKS 500


int old_data_reads, old_data_writes, old_inode_reads, old_inode_writes;
int new_data_reads, new_data_writes, new_inode_reads, new_inode_writes;
struct disk_stat *before, *after;
char *names[] = { "f0", "f1", "f2", "f3", "f4" };

int
main(int argc, char *argv[])
{
    printf(1, "workload_1 test starting\n");
    int fd;
    char buf[512];
    int i;
    int j;
    int k;
    int num_blocks, filenum;

    if (report_stats(before) < 0){
        printf(2, "error reading stats");
        exit();
    }
    printf(1, "Before:");
    old_data_reads = before->disk_data_read_count;
    old_data_writes = before->disk_data_write_count;
    old_inode_reads = before->disk_inode_read_count;
    old_inode_writes = before->disk_inode_write_count;
    printf(1, "\nDisk data reads: %d", before->disk_data_read_count);
    printf(1, "\nDisk inode reads: %d", before->disk_inode_read_count);
    printf(1, "\nDisk data writes: %d", before->disk_data_write_count);
    printf(1, "\nDisk inode writes: %d", before->disk_inode_write_count);
    printf(1, "\nNumber of free blocks on disk: %d\n", before->free_blocks);


    // Create 4 small files with 100 blocks each, and 1 big file with 500 blocks, then write to them in order SMALL SMALL SMALL SMALL BIG, 5 writes each. Write in cache-friendly manner; write all the way through to one, then move on. 
    for (k = 0; k < NSMALLFILES + NBIGFILES; k++) {
        
        fd = open(names[k], O_CREATE | O_RDWR);
        if (k < NSMALLFILES)
            num_blocks = SMALLNBLOCKS;
        else
            num_blocks = BIGNBLOCKS;
        for (i = 0; i < num_blocks; i++) {
            for (j = 0; j < 512; j++) {
                buf[j] = i;
            }
            write(fd, buf, 512);
        }
        close(fd);
    }

    printf(1, "created files; ok\n");

    for (k = 0; k < 5 * (NSMALLFILES + NBIGFILES); k++) {
        filenum = k / 5;
        fd = open(names[filenum], O_RDWR);
        if (filenum < NSMALLFILES)
            num_blocks = SMALLNBLOCKS;
        else
            num_blocks = BIGNBLOCKS;
        for (i = 0; i < num_blocks; i++) {
            for (j = 0; j < 512; j++) {
                buf[j] = k;
            }
            write(fd, buf, 512);
        }
        close(fd);
    }

    printf(1, "wrote to files; ok\n");

    for (int k=0; k < 5; k++){
        if(unlink(names[k]) < 0){
            printf(2, "unlink %s failed\n", names[k]);
            exit();
        }
    }

    printf(1, "removed files; ok\n");

    //Report stats
    if (report_stats(after) < 0){
        printf(2, "error reading stats");
        exit();
    }

    new_data_reads = after->disk_data_read_count;
    new_data_writes = after->disk_data_write_count;
    new_inode_reads = after->disk_inode_read_count;
    new_inode_writes = after->disk_inode_write_count;

    printf(1, "After:");
    printf(1, "\nDisk data reads: %d", after->disk_data_read_count);
    printf(1, "\nDisk inode reads: %d", after->disk_inode_read_count);
    printf(1, "\nDisk data writes: %d", after->disk_data_write_count);
    printf(1, "\nDisk inode writes: %d", after->disk_inode_write_count);
    printf(1, "\nNumber of free blocks on disk: %d\n", after->free_blocks);

    printf(1, "Summary:");
    printf(1, "\nWorkload disk data reads: %d", new_data_reads - old_data_reads);
    printf(1, "\nWorkload disk inode reads: %d", new_inode_reads - old_inode_reads);
    printf(1, "\nWorkload disk data writes: %d", new_data_writes - old_data_writes);
    printf(1, "\nWorkload disk inode writes: %d\n", new_inode_writes - old_inode_writes);

    printf(1, "workload_1 test OK\n");
    exit();
}