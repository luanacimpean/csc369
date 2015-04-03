Start -> disk
Data -> img_disk
K->i
2 (inode_num) -> EXT2_ROOT_INO


onefile.img has:
•	test.txt
•	test2.txt

mystat → s_1
mystat_2 → s_2

fd → fd_1
fd_2 → fd_2 (stays the same!)

(struct)
block_group -> ext2_group_desc
uint32_t → unsigned int
uint16_t →unsigned short
uint8_t -> unsigned char

(struct)
inode → ext2_inode

inode_table -> bg_inode_table

d_blocks -> i_block

REGULAR_FILE -> EXT2_BAD_INO

inode_bitmap -> bg_inode_bitmap

(struct ext2_group_desc)
inode_num -> bg_free_inodes_count
block_bitmap-> bg_block_bitmap
block_num-> bg_free_blocks_count

(struct ext2_super_block)
uinode_num -> s_free_inodes_count
ublocks_num ->s_free_blocks_count

struct block_entry -> struct ext2_dir_entry_2
dir_len-> rec_len
inode_num-> inode
