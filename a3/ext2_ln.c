#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//char *filename;
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <inttypes.h>
#include "ext2.h"



#define BLOCK_SIZE 1024

/*Got this from:
 stackoverflow.com/questions/2420780/how-to-assign-a-value-to-a-char-using-hex-notation
  By Claudiu
*/
#define CHECK_BIT(var,pos) ((var) &(1<<pos))

int find_inode(char *start, char *name, int block_num,int checker,char *name_file,int go_start);



int main(int argc,char *argv[]){
        char path_1[255];
        char path_2[255];
        struct stat mystat;
        if (argc != 4){
                perror("Wrong number of arguments, Please enter again\n");
                exit(1);

        }
        int fd = open(argv[1],O_RDWR);
        strncpy(path_1,argv[2],255);
        strncpy(path_2,argv[3],255);
        if (fd == 1){
                perror("open");
                exit(1);
        }
        if (fstat(fd,&mystat)< 0){
                perror("stat");
                exit(1);
        }


        char *start;
        start= mmap(0,mystat.st_size,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
        if (start == MAP_FAILED){
                perror("mapping failed\n");
                exit(1);
        }

        //get the super block
        struct ext2_super_block *super_block = malloc(sizeof(struct ext2_super_block));
        memcpy(super_block, start+1024,sizeof(struct ext2_super_block));
        //get this block_group
        struct ext2_group_desc *block_group = malloc(sizeof(struct ext2_group_desc));
        memcpy(block_group,start+2048,sizeof(unsigned int)*3 + sizeof(unsigned short)*3);

        /*
        1. find the inode for the first path.
        2. make the file for the second path.3
        3. link them
        */

        //Path 1
        int len = strlen(path_1);
        path_1[len] = '\0';
        int checker = 1;
        int k;
        int counter = 0;
        for (k=0;k<strlen(path_1);k++){
                if (path_1[k] == '/'){
                        counter = counter + 1;
                }
        }
        //set an array of string for all indices
                                                   
        char result[counter][255];

        int index = 0;
        int b = 0;
        //put the name of indices into the array
        for (b = 1; b<len;b++){
                if (path_1[b] == '/'){
                        memcpy(result[index], &path_1[checker],b-checker);
                        result[index][b-checker] = '\0';
                        checker = b +1;
                        index= index + 1;
                }
        }
        memcpy(result[index],&path_1[checker],b - checker);
        result[index][b-checker] = '\0';

        //Path 2
        int len_2 = strlen(path_2);
        path_2[len_2] = '\0';
        int checker_2 = 1;
        int k_2;
        int counter_2 = 0;
        for (k_2=0;k_2<strlen(path_2);k_2++){
                if (path_2[k_2] == '/'){
                        counter_2 = counter_2 + 1;

                }

        }
        char result_2[counter_2][255];

        int index_2 = 0;
        int b_2 = 0;
        for (b_2 = 1; b_2 <len_2;b_2++){
                if (path_2[b_2] == '/'){
                        memcpy(result_2[index_2], &path_2[checker_2],b_2-checker_2);
                        result_2[index_2][b_2-checker_2] = '\0';
                        checker_2 = b_2 +1;
                        index_2= index_2 + 1;

                }

        }
        memcpy(result_2[index_2],&path_2[checker_2],b_2 - checker_2);
        result_2[index_2][b_2-checker_2] = '\0';

        //find the inode for the first path
        index = 0;
        int inode_num = 2;
        while(index < counter){
                struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
                memcpy(inode,start+(block_group->bg_inode_table*1024+ 128 *(inode_num-1)),sizeof(struct ext2_inode));

                int block_index = 0;
                int temp = -1;

                while(inode->i_block[block_index]){

                        uint32_t i_num = inode->i_block[block_index];

                        temp = find_inode(start,result[index],i_num,0,result[counter-1],0);

                        if (temp!= 1){
                                inode_num = temp;
                                break;
                        }
                        block_index = block_index + 1;
                }
                if (temp == 1 || temp == -1){
                        perror("The first path doesn't appear\n");
                        exit(1);
                }
                index = index + 1;

        }
        //need to check if is a directory
        struct ext2_inode *inode_testing = malloc(sizeof(struct ext2_inode));
        memcpy(inode_testing,start+(block_group->bg_inode_table *1024+ 128 *(inode_num-1)),sizeof(struct ext2_inode));
        unsigned char mode = inode_testing->i_mode;
        mode = inode_testing->i_mode >> 12;
        if (mode == 8 ){
        }else if (mode == 4 ){
                perror("Directory\n");
                exit(1);

        }

        //check if path 2 already exists
        index_2= 0;
        int inode_num_2 = 2;
        int temp_2;
        while(index_2 < counter_2){
                struct ext2_inode *inode_2 = malloc(sizeof(struct ext2_inode));
                memcpy(inode_2,start+(block_group->bg_inode_table*1024+ 128 *(inode_num_2-1)),sizeof(struct ext2_inode));

                int block_index_2 = 0;

                while(inode_2->i_block[block_index_2]){
                        unsigned int i_num_2 = inode_2->i_block[block_index_2];

                        temp_2 = find_inode(start,result_2[index_2],i_num_2,0,result_2[counter_2-1],0);

                        if (temp_2!= 1){

                                inode_num_2= temp_2;
                                break;
                        }
                        block_index_2 = block_index_2 + 1;
                }
                index_2 = index_2 + 1;

        }
        if (temp_2 == 1 || temp_2 == -1){

        }else{
                perror("Path 2 already exits, stop\n");
                exit(1);

        }
        //find a space for entry block of path 2
        index_2= 0;
        inode_num_2 = 2;
        while(index_2 < counter_2-1){
                struct ext2_inode *inode_2 = malloc(sizeof(struct ext2_inode));
                memcpy(inode_2,start+(block_group->bg_inode_table*1024+ 128 *(inode_num_2-1)),sizeof(struct ext2_inode));

                int block_index_2 = 0;
                int temp_2 = -1;

                while(inode_2->i_block[block_index_2]){
                        uint32_t i_num_2 = inode_2->i_block[block_index_2];

                        temp_2 = find_inode(start,result_2[index_2],i_num_2,0,result_2[counter_2-1],0);

                        if (temp_2!= 1){

                                inode_num_2= temp_2;
                                break;
                        }
                        block_index_2 = block_index_2 + 1;
                }
                if (temp_2 == 1 || temp_2 == -1){
                        perror("The path 2 doesn't exist\n");
                        exit(1);
                }
                index_2 = index_2 + 1;

        }

        //linked the new inode into correct directory
        struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));

        memcpy(inode,start+(block_group->bg_inode_table *1024+ 128 *(inode_num_2-1)),sizeof(struct ext2_inode));

        //and this inode has to be a directory, then we can put file in it.
        //go to the block entry of this inode.
        int block_index = 0;
        int temp = -1;
        while(inode->i_block[block_index]){
                int i_num = inode->i_block[block_index];

                temp = find_inode(start,"",i_num,1,result_2[counter_2-1],inode_num);
                if (temp != -1){
                        break;
                }
                block_index += 1;
        }
        return 0;
}


/*The function that for helping find the correct inode
number from the correct block */
int find_inode(char *start,
char *name,
int block_num,
int checker,
char *name_file,
int go_start){

        //using block_entry structure to find the inode of correct name
        int begin = 0;
        char temp[255];
        int len = sizeof(unsigned int) + sizeof(unsigned short) +sizeof(char) *2;
        while(begin <= 1024){
                struct ext2_dir_entry_2 *block_entry = malloc(sizeof(struct ext2_dir_entry_2));
                memcpy(block_entry, start+(block_num * 1024)+ begin,len);
                memcpy(temp,start+(block_num*1024)+begin+len,block_entry->name_len);

                memcpy(block_entry->name,temp, block_entry->name_len);
                if (strcmp(block_entry->name,name) == 0){

                        /*checker means that we need to update this entry in order to add new file*/
                        if (checker == 1){
                ;
                                memcpy(block_entry->name,name_file,strlen(name_file));
                                block_entry->rec_len = 1024 - begin;
                                block_entry->name_len = strlen(name_file);
                                block_entry->file_type = SYMBOLIC_LINK;
                                block_entry->inode = go_start;
                                memcpy(start+(block_num *1024)+begin,block_entry,len+block_entry->name_len);
                        //      return 0;
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
