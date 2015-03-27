#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *load_image(char *image_path){
    unsigned char *disk;

    int fd = open(image_path, O_RDWR);
    if (fd == -1){
        fprintf(stderr, "Cannot load disk image from '%s'.\n", image_path);
        exit(1);
    }

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    close(fd);

    return disk;

}


int main(int argc, char **argv) {

    if(argc != 3) {
        fprintf(stderr, "Usage: ext2_ls <image file name> <absolute path>\n");
        exit(1);
    }


    // Load the image
    unsigned char *disk = load_image(argv[1]);
    printf("Disk is loaded to memory at address %p.\n", disk);


    // For now, let's try to list the root directory.

    // NOTE: Please use EXT2_ROOT_INO instead of the "magic" number 2.
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2048);

    printf("\tinode table: %d\n", bg->bg_inode_table);

    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + (bg->bg_inode_table * 1024));   // TODO: Use a constant, not 1024

    struct ext2_inode root_inode = inode_table[1];
    printf("The i_size field of the root inode is %d.\n", root_inode.i_size);

    // TODO: Can we list directories with root inode?


    // TODO: Find the directory with the given path......

    // TODO: List all directory entries from the directory.


    return 0;

}                                                            1,1           Top
