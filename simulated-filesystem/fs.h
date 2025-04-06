#ifndef FS_H
#define FS_H

#define INODE_MAX 1024
#define ARG_MAX 32
#define inode_List "inodes_list"

typedef struct 
{
    uint32_t inode;
    uint32_t parentInode;
    char type;
    char name[ARG_MAX];
} Inode;

typedef struct
{
    Inode *inode; // the actual inodes
    int capacity;  // number of current inodes
} InodeArray;


int loadInodeList(FILE *inodeList);
void saveInodeList(const char *path);
void changeDirectory(const char *name);
void listContents();
void createDirectory(const char *name);
void createFile(const char *name);
void createInodeArr(InodeArray* arr);
void addInode(InodeArray* arr);
void insertInode(InodeArray* arr, Inode inode);
void printAllInodes(InodeArray* arr);
void runPrompt(InodeArray *arr);
char *getName(int start, char*str);
char *uint32_to_str(uint32_t i);

#endif