#include "lab4_ag.h"
#include "defs.h"

static int disk_data_read_count = 0;
static int disk_data_write_count = 0;
static int disk_inode_read_count = 0;
static int disk_inode_write_count = 0;
static int free_blocks = 0;

void report_disk_data_read(){
    disk_data_read_count++;
}

void report_disk_inode_read(){
    disk_inode_read_count++;
}

void report_disk_data_write(){
    disk_data_write_count++;
}

void report_disk_inode_write(){
    disk_inode_write_count++;
}

int get_free_blocks(){
    return -1;
}

int report_stats(struct disk_stat *dstat){
    // cprintf("\nDisk data reads: %d", disk_data_read_count);
    // cprintf("\nDisk inode reads: %d", disk_inode_read_count);
    // cprintf("\nDisk data writes: %d", disk_data_write_count);
    // cprintf("\nDisk inode writes: %d", disk_inode_write_count);
    // cprintf("\nNumber of free blocks on disk: %d\n", free_blocks);z
    dstat->disk_data_read_count = disk_data_read_count;
    dstat->disk_data_write_count = disk_data_write_count;
    dstat->disk_inode_read_count = disk_inode_read_count;
    dstat->disk_inode_write_count = disk_inode_write_count;
    dstat->free_blocks = get_free_blocks();
    return 0;
}