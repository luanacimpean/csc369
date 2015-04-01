#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <inttypes.h>
#include "ext2_cp.h"
#include "ext2.h"

#define BLOCK_SIZE 1024

/*Credit:
stackoverflow.com/questions/2420780/how-to-assign-a-value-to-a-char-using-hex-notation
By Claudiu
*/
#define CHECK_BIT(var,pos) ((var) &(1<<pos))

/*Loads disk image*/
char *load_image(char *image_path);

/*Find the correct inode number from the correct block */
int find_inode_num(char *disk, char *name, int block_num, int flag, char *name_file, int go_start);


/*MAIN.
arv[0] = function name
arv[1] = name of ext2 formatted disk
arv[2] = absolute path to ext2 formatted disk
 */
int main(int argc, char *argv[]){

        char path[255];

        if (argc != 3){
                perror("Error: Incorrect number of arguments.");
                exit(1);
        }
        strncpy(path, argv[2], 255); //copy given path

        char *disk = load_image(argv[1]); //load given disk image

        //Find superblock
        struct ext2_super_block *super_block = malloc(sizeof(struct ext2_super_block));
        memcpy(super_block, disk + 1024, sizeof(struct ext2_super_block));

        //Find first block group
        struct block_group *block_group = malloc(sizeof(struct block_group));
        memcpy(block_group, disk + 2048, sizeof(uint32_t)*3 + sizeof(uint16_t)*3);

        //Caculate the number of indices in given path
        int path_len = strlen(path);
        path[path_len] = '\0';
        int i;
        int counter = 0;
        int flag = 1;
        for (i=0; i<path_len; i++){
                if (path[i] == '/'){
                        counter = counter + 1;
                }
        }

        //set an array of strings for all indices
        char result[counter][255];
        int idx = 0;
        int j = 0;

        //put the name of indices into the array
        for (j = 1; j<path_len; j++){
                if (path[j] == '/'){
                        memcpy(result[idx], &path[flag], j-flag);
                        result[idx][j-flag] = '\0';
                        flag = j + 1;
                        idx= idx + 1;
                }
        }
        memcpy(result[idx], &path[flag], j-flag);
        result[idx][j-flag] = '\0';

        //check if this directory already exist
        idx = 0;
        //start from the root inode
        int inode_num = EXT2_ROOT_INO;
        int temp = -1;
        while(idx < counter){
                //go to root inode.
                struct inode *inode = malloc(sizeof(struct inode));
                memcpy(inode, disk + (block_group->inode_table*1024+ 128 *(inode_num-1)), sizeof(struct inode));

                int block_index = 0;
                int temp = 1;


                //checking all data blocks
                while(inode->d_blocks[block_index]){
                        uint32_t i_num = inode->d_blocks[block_index];
                        temp = find_inode_num(disk, result[idx],i_num,0,result[counter-1],0);
                        //find it
                        if (temp!= 1){

                                inode_num = temp;
                                break;
                        }
                        block_index = block_index + 1;
                }
                idx = idx + 1;

        }
        //do the checking of existence
        if (temp == 1 || temp == -1){
        }else{
                perror("Directory already exits, stop\n");
                exit(1);
        }
        //find the correct space
        idx = 0;
        inode_num = 2;
        while(idx < counter-1){
                struct inode *inode = malloc(sizeof(struct inode));
                memcpy(inode, disk + (block_group->inode_table*1024+ 128 *(inode_num-1)), sizeof(struct inode));

                int block_index = 0;
                int temp = -1;

                while(inode->d_blocks[block_index]){
                        uint32_t i_num = inode->d_blocks[block_index];

                        temp = find_inode_num(disk, result[idx], i_num,0, result[counter-1], 0);
                        if (temp!= 1){

                                inode_num = temp;
                                break;
                        }
                        block_index = block_index + 1;
                }
                if (temp == 1 || temp == -1){
                        perror("The path is something wrong\n");
                        exit(1);
                }
                idx = idx + 1;

        }


        //find new free inode space
        int inode_bitmap = block_group->inode_bitmap;
        unsigned char i_bitmap[1024];
        memcpy(i_bitmap, disk + inode_bitmap *1024,1024);
        int k;
        int go_start = 0;
        for(k = 0; k<1023; k++){
                if (i_bitmap[k] != 0xff){
                        go_start = (k+1) *8 -7;
                        int in;
                        //do the bit checking
                        for (in = 0;in <=7; in++){
                                if(CHECK_BIT(i_bitmap[i],in)){
                                        go_start = go_start + 1;
                                }else{
                                 //set this bit
                                        block_group->inode_num -=1;
                                        super_block->uinodes_num -=1;
                                        i_bitmap[i] |= 1 << in;
                                        break;
                                }
                        }
                        break;
                }

        }
        if (go_start == 0){
                perror("No enough inodes\n");
                exit(1);
        }
        memcpy(disk + inode_bitmap * 1024,i_bitmap,1024);
        memcpy(disk + 2048, block_group, sizeof(uint32_t)*3 + sizeof(uint16_t)*3);
        memcpy(disk + 1024, super_block, sizeof(struct ext2_super_block));


        //linked the new inode into correct directory
        struct inode *inode = malloc(sizeof(struct inode));

        memcpy(inode, disk +(block_group->inode_table *1024+ 128 *(inode_num-1)), sizeof(struct inode));

        //and this inode has to be a directory, then we can put file in it.
        //go to the block entry of this inode.
        int block_index = 0;
        temp = -1;
        while(inode->d_blocks[block_index]){
                int i_num = inode->d_blocks[block_index];

                temp = find_inode_num(disk, "", i_num, 1, result[counter-1], go_start);
                if (temp != -1){
                        //printf("Success\n");
                        break;
                }
                block_index += 1;
        }

        //set the type of this inode
         inode->type =0x4000;
        memcpy(disk + (block_group->inode_table *1024+ 128 *(inode_num-1)), inode, sizeof(struct inode));

        //close(fd);
        free(inode);
        free(super_block);
        free(block_group);

        return 0;
}



/*Loads disk image*/
char *load_image(char *image_path){

        char *disk;
        struct stat mystat;
        off_t ret;

        int fd = open(image_path, O_RDWR); //open disk

        /*ERROR CHECK*/
        if (fd == 1){
                fprintf(stderr, "Cannot load disk image from '%s'.\n", image_path);
                exit(1);
        }

        if ((ret = lseek(fd, 1024, SEEK_SET) ==(off_t)-1)){
                perror("Error: lseek fail");
                exit(1);
        }

        if (fstat(fd, &mystat) < 0){
                perror("Error: stat fail");
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

/*Find the correct inode number from the correct block */
int find_inode_num(char *disk, char *name, int block_num, int checker, char *name_file, int go_start){

        //using block_entry structure to find the inode of correct name

        int begin = 0;
        char temp[255];
        int len = sizeof(uint32_t) + sizeof(uint16_t) +sizeof(uint8_t) *2;
        while(begin <= 1024){
                struct block_entry *block_entry = malloc(sizeof(struct block_entry));
                memcpy(block_entry, disk + (block_num * 1024)+ begin,len);
                memcpy(temp, disk + (block_num*1024) + begin + len, block_entry->name_len);
                memcpy(block_entry->name,temp, block_entry->name_len);
                if (strcmp(block_entry->name,name) == 0){
                        /*checker means that we need to update this entry in order to add new file*/
                        if (checker == 1){
                                memcpy(block_entry->name,name_file,strlen(name_file));
                                block_entry->dir_len = 1024 - begin;
                                block_entry->name_len = strlen(name_file);
                                block_entry->file_type = DIRECTORY;
                                block_entry->inode_num = go_start;
                                memcpy(disk + (block_num *1024) + begin, block_entry, len + block_entry->name_len);
                        }
                        return block_entry->inode_num;
                }else if(block_entry->name_len == 0) {
                        return 1;



                }
                if (block_entry->dir_len + 24 == 1024){
                        begin = begin + block_entry->name_len + 8 -1;
                }else{
                        begin = begin + block_entry->dir_len;
                }
        }
        return 1;
}

                                                               163,1-8       48%

