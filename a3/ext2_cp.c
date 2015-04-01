#include <stdio.h>
#include <stdlib.h>
//#include "ext2_cp.h"
#include "ext2.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <inttypes.h>


#define BLOCK_SIZE 1024

// stackoverflow.com/questions/2420780/how-to-assign-a-value-to-a-char-using-hex-notation
#define CHECK_BIT(var,pos) ((var) &(1<<pos))

// a function that finds the inode number from the block number
int find_inode(char *disk, char *name, int block_num, int checker, char *name_file, int go_start);


int main(int argc,char *argv[]){

        char path[255];
        struct stat s_1;
        struct stat s_2;

        if (argc != 4){
                perror("Please enter 4 arguments\n");
                exit(1);
        }

        int fd_1 = open(argv[1],O_RDWR);
        int fd_2 = open(argv[2],O_RDWR);
        strncpy(path,argv[3],255);

        if (fd_1 == 1){
                perror("open fd_1");
                exit(1);
        }

        if (fd_2 == 1){
                perror("open fd_2");
                exit(1);
        }

        if (fstat(fd_1,&s_1)< 0){
                perror("stat 1");
                exit(1);
        }

        if (fstat(fd_2,&s_2)<0){
                perror("stat 2");
                exit(1);
        }

        unsigned char *disk;
        disk = mmap(0, s_1.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_1, 0);
        if (disk == MAP_FAILED){
                perror("mmap");
                exit(1);
        }

        unsigned char *img_disk;
        img_disk = mmap(0, s_2.st_size, PROT_READ, MAP_SHARED, fd_2, 0);
        if (img_disk == MAP_FAILED){
                perror("mmap");
                exit(1);


        }

        // get the super block
        struct ext2_super_block *super_block = malloc(sizeof(struct ext2_super_block));
        memcpy(super_block, disk+1024,sizeof(struct ext2_super_block));

        // get this block_group 
        struct ext2_group_desc *block_group = malloc(sizeof(struct ext2_group_desc));
        memcpy(block_group,disk+2048,sizeof(unsigned int)*3 + sizeof(unsigned short)*3);

        // find out where index of file is
        int len = strlen(path);
        path[len] = '\0';
        int checker = 1;
        int k;
        int counter = 0;
        for (k=0;k<strlen(path);k++){
                if (path[k] == '/'){
                        counter = counter + 1;
                }
        }

        // create an array to store dir or file names
        char result[counter][255];
        int index = 0;
        int b = 0;

        // put the name filenames into the array
        for (b = 1; b<len;b++){
                if (path[b] == '/'){
                        memcpy(result[index], &path[checker],b-checker);
                        result[index][b-checker] = '\0';
                        checker = b +1;
                        index= index + 1;
                }

        }

        memcpy(result[index],&path[checker],b - checker);
        result[index][b-checker] = '\0';

        index = 0;
        int inode_num = EXT2_ROOT_INO;

        while(index < counter-1){
                struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
                memcpy(inode,disk+(block_group->bg_inode_table*1024+128*(inode_num-1)),sizeof(struct ext2_inode));

                int block_index = 0;
                int temp = -1;
                while(inode->i_block[block_index]){
                        unsigned int i_num = inode->i_block[block_index];
                        temp = find_inode(disk,result[index],i_num,0,result[counter-1],0);
                        if (temp!= 1){
                                // prev_inode = inode_num;
                                inode_num = temp;
                                break;
                        }
                        block_index = block_index + 1;
                }
                if (temp == 1 || temp == -1){
                        perror("path does not exist");
                        exit(1);
                }
                index = index + 1;
        }


        // find new inode space
        int bg_inode_bitmap = block_group->bg_inode_bitmap;
        unsigned char i_bitmap[1024];
        memcpy(i_bitmap,disk+bg_inode_bitmap *1024,1024);
        int i;
        int go_start = 0;
        for(i = 0;i<1023;i++){
                if (i_bitmap[i] != 0xff){
                        go_start = (i+1) * 8 - 7;
                        int in;
                        for (in=0;in<=7;in++){
                                if(CHECK_BIT(i_bitmap[i],in)){
                                        go_start = go_start + 1;
                                }else{
                                        //set this bit
                                        block_group->bg_free_inodes_count -=1;
                                        super_block->s_free_inodes_count -= 1;
                                        i_bitmap[i] |= 1 << in;
                                        break;
                                }
                        }
                        break;
                }
        }
        if (go_start == 0){
                printf("No enough inodes\n");
                exit(1);
        }
        memcpy(disk+bg_inode_bitmap * 1024,i_bitmap,1024);
        //count how many blocks we need to store those data
        int how_many_blocks = 1;
        int s = s_2.st_size;
        while (s > 1024){
                how_many_blocks +=1;
                s = s - 1024;
        }
        struct ext2_inode *new_inode = malloc(sizeof(struct ext2_inode));
        memcpy(new_inode,disk+(block_group->bg_inode_table *1024+128 *(go_start-1)),sizeof(struct ext2_inode));

        //find new block space
        int block_bitmap = block_group->bg_block_bitmap;
        unsigned char b_bitmap[1024];
        memcpy(b_bitmap,disk+block_bitmap *1024,1024);
        int block_space = 0;
        int c = 0;
        while (how_many_blocks >0){
                for(i = 0;i<1023;i++){
                        if (b_bitmap[i] != 0xff){
                                block_space= (i+1) *8 -7;
                                int in;
                                for (in = 0;in <=7; in++){
                                        if(CHECK_BIT(b_bitmap[i],in)){
                                                block_space = block_space + 1;
                                        }else{
                                                new_inode->i_block[c] = block_space;
                                                memcpy(disk + block_space * 1024,img_disk+c,1024);
                                                block_group->bg_free_blocks_count -=1;
                                                super_block->s_free_blocks_count -=1;
                                                b_bitmap[i] |= 1 << in;
                                                break;
                                        }
                                }
                                break;
                        }
                }
                c = c + 1;
                how_many_blocks -= 1;
        }
        //update super_block,block_group and bitmaps.
        memcpy(disk+block_bitmap * 1024,b_bitmap,1024);
        memcpy(disk + (block_group->bg_inode_table * 1024 + 128 *(go_start-1)),new_inode,sizeof(struct ext2_inode));
        memcpy(disk+2048,block_group,sizeof(unsigned int)*3 + sizeof(unsigned short)*3);
        memcpy(disk+1024,super_block,sizeof(struct ext2_super_block));

        //link the new inode into correct directory
        struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));

        memcpy(inode,disk+(block_group->bg_inode_table *1024+ 128 *(inode_num-1)),
                sizeof(struct ext2_inode));

        //and this inode has to be a directory, then we can put file in it.
        //go to the block entry of this inode.
        int block_index = 0;
        int temp = -1;
        while(inode->i_block[block_index]){
                int i_num = inode->i_block[block_index];

                temp = find_inode(disk,"",i_num,1,result[counter-1],go_start);
                if (temp != -1){
                        break;
                }
                block_index += 1;
        }

        close(fd_2);
        close(fd_1);
        return 0;
}


// finds the inode number from the block number
int find_inode(char *disk, char *name, int block_num, int checker, char *name_file, int go_start){

        //use block_entry structure to find the inode of correct name
        int begin = 0;
        char temp[255];
        int len = sizeof(unsigned int) + sizeof(unsigned short) + sizeof(unsigned char) * 2;

        //search the blocks using the block entry structure
        while(begin <= 1024){
                struct ext2_dir_entry_2 *block_entry = malloc(sizeof(struct ext2_dir_entry_2));
                memcpy(block_entry, disk+(block_num * 1024)+ begin,len);
                memcpy(temp,disk+(block_num*1024)+begin+len,block_entry->name_len);

                memcpy(block_entry->name,temp, block_entry->name_len);

                if (strcmp(block_entry->name,name) == 0){

                        /*checker means that we need to update this entry 
                        in order to add new file*/
                        if (checker == 1){

                                memcpy(block_entry->name,name_file,strlen(name_file));
                                block_entry->rec_len = 1024 - begin;
                                block_entry->name_len = strlen(name_file);
                                block_entry->file_type = EXT2_BAD_INO;
                                block_entry->inode = go_start;
                                memcpy(disk+(block_num *1024)+begin,block_entry,
                                        len+block_entry->name_len);

                        }
                        return block_entry->inode;
                }else if(block_entry->name_len == 0) {
                        return 1;


                }
                if (block_entry->rec_len + 24 == 1024){
                        begin = begin + block_entry->name_len + 8 -1;
                }else{
                        begin = begin + block_entry->rec_len;
                }
        }


        return 1;
}
