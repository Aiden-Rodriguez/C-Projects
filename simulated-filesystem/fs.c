#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "fs.h"


InodeArray inodeArr;
char *inputStr = NULL;
uint32_t currentDirInode;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Required cmd argument of filesys");
        return 0;
    }

    //pretty much just fs or empty.
    int checkchdir = chdir(argv[1]);
    if (checkchdir == -1)
    {
        printf("Unknown file system");
        return 0;
    }

    //open the inodelist, read binary for it so we can store its info
    FILE *inodeList = fopen(inode_List, "rb");

    //Handle empty cases (empty dir)
    if (inodeList == NULL)
    {
        printf("inode_List file does not exist or cannot");
        printf("be opened for reading. Attempting to create...\n");

        // Attempt to create the file
        inodeList = fopen(inode_List, "wb");
        if (inodeList == NULL) 
        {
            printf("Failed to create the file.\n");
            exit(1);
        }
        else 
        {
            printf("File created successfully.\n");
            fclose(inodeList);
            inodeList = fopen(inode_List, "rb");
            if (inodeList == NULL)
            {
                printf("Failed to open newly created file\n");
                exit(1);
            }
            
        }
    }

    //create the inodearr, and load stuff into it from file
    createInodeArr(&inodeArr);
    int inodeCheck = loadInodeList(inodeList);
    //printf("Inode Check!\n");
    //printAllInodes(&inodeArr);

    //This is listed as a requirement under req2
    //Shouldnt really happen unless inodes_list is somehow manually edited
    if (inodeCheck == 0)
    {
        if (inodeArr.inode[0].type != 'd')
        {
            printf("inode 0 is not a directory... womp womp\n");
            exit(1);
        }
    }
    //make root dir upon creating new inode_list file
    else if (inodeCheck == 1)
    {
        //Fix inode data!
        inodeArr.inode[0].inode = 0;
        strcpy(inodeArr.inode[0].name, "root");
        inodeArr.inode[0].parentInode = 0;
        inodeArr.inode[0].type = 'd';
        
        //Make the physical inode file
        FILE *fp = fopen("0", "w");
        if (fp == NULL) 
        {
            perror("Error opening file");
            exit(1);
        }
        fprintf(fp, "0 .\n0 ..");
        fclose(fp);        
    }

    //Pretty much running the meat of the program
    //Wait for user input in an infinite loop until exit condition
    currentDirInode = 0;
    runPrompt(&inodeArr);

    //cleaning up
    fclose(inodeList);
    free(inputStr);
    free(inodeArr.inode);

    //printAllInodes(&inodeArr);

    return 0;
}

//Create inode arr, allocate 1 slot for a node.
void createInodeArr(InodeArray* arr)
{
    arr->inode = (Inode *)malloc(1 * sizeof(Inode));
    if (arr->inode == NULL)
    {
        perror("Malloc Error");
        exit(1);
    }
    arr->capacity = 1;
}

//create slot for node
void addInode(InodeArray* arr)
{
    Inode* tmp = (Inode *)realloc(arr->inode, (1 + arr->capacity)* 
    sizeof(Inode));
    if (tmp == NULL)
    {
        perror("Realloc Error");
        exit(1);
    }
    arr->inode = tmp;
    arr->capacity++;
}

//insert a node into array
void insertInode(InodeArray* arr, Inode inode)
{
    arr->inode[arr->capacity-1] = inode;
}

//test function thats prints every inode reguardless
void printAllInodes(InodeArray* arr)
{
    int i;
    for (i = 0; i < arr->capacity; i++) {
        printf("Inode: %u\n", arr->inode[i].inode);
        printf("Parent Inode: %u\n", arr->inode[i].parentInode);
        printf("Type: %c\n", arr->inode[i].type);
        printf("Name: %s\n", arr->inode[i].name);
    }
}

//Load the inodes into the inodeArr struct, so we can do things
//with them
//Returns 1 if inodeList loading is on a newly created one.
int loadInodeList(FILE *inodeList) {
    //insert inode list into array :D
    Inode tmpNode;
    int chkVal = 0;
    int check = 0;
    int loopCheck = 0;
    while (chkVal != -1)
    {
        if ((chkVal = fread(&tmpNode, sizeof(Inode), 1, inodeList)) != 1)
        {
            if (fgetc(inodeList) == EOF) 
            {
                // End of file reached, break the loop
                if (check == 0)
                {
                    return 1;
                }
                break;
            }  
            else
            {
                perror("fread failure");
                exit(1);
            }
        }
        //check if over max inodes
        else if (inodeArr.capacity < INODE_MAX)
        {

            //only run this for the 1st
            if (check == 0)
            {
                insertInode(&inodeArr, tmpNode);
                check = 1;
            }
            else
            {
                addInode(&inodeArr);
                insertInode(&inodeArr, tmpNode);
            }
        }
        //over max inodes read for inodes_list. idk when this would happen, 
        //but just incase. 
        else
        {
            printf("inode file seems to be over the max permitted inodes");
            printf("... filesystem will not work\n");
            exit(1);
        }
    }
    return loopCheck;
}

void runPrompt(InodeArray *arr)
{
    size_t len = 0;
    ssize_t nRead = 0;
    int strLen;
    char* name;
    // go in infinite loop until an exit condition occurs. Read input,
    // do commands based upon that input
    while (1)
    {
        printf("> ");
        nRead = getline(&inputStr, &len, stdin);
        //check for exit conditions (EOF)
        if (nRead == -1) 
        {
            saveInodeList(inode_List);
            printf("Exiting\n");
            return;
        }
        strLen = strlen(inputStr);
        //cd check
        if (strLen > 4 && strncmp(inputStr, "cd ", 3) == 0)
        {
            name = getName(4, inputStr);
            if (name != NULL)
            {
                changeDirectory(name);
                free(name);
            }
            else
            {
                printf("Bad directory name; Use max of 32 chars\n");
            }
        }
        //mkdir check
        else if (strLen > 7 && strncmp(inputStr, "mkdir ", 6) == 0)
        {
            name = getName(7, inputStr);
            if (name != NULL)
            {
                createDirectory(name);
                free(name);
            }
            else
            {
                printf("Bad directory name; Use max of 32 chars\n");
            }
        }
        //touch check
        else if (strLen > 7 && strncmp(inputStr, "touch ", 6) == 0)
        {
            name = getName(7, inputStr);
            if (name != NULL)
            {
                createFile(name);
                free(name);
            }
            else
            {
                printf("Bad file name; Use max of 32 chars\n");
            }
        }
        //exit check
        else if (strLen == 5)
        {
            if (strcmp(inputStr, "exit\n") == 0)
            {
                printf("Exiting\n");
                break;
            }
            else
            {
                printf("Unknown Command\n");
            }
        }
        //ls check
        else if (strLen == 3)
        {
                    
            if (strcmp(inputStr, "ls\n") == 0)
            {
                listContents();
            }
            else
            {
                printf("Unknown Command\n");
            }
        }
        //anything else
        else
        {
            printf("Unknown Command or incorrect usage of command\n");
        }
    }
    //free(name);
    saveInodeList(inode_List);
    return;
}

// write the content of the inodeList to the file path
void saveInodeList(const char *path) {
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) 
    {
        perror("Error opening file");
        exit(1);
    }
    Inode tmpnode;
    int i;

    // Initialize tmpnode because valgrind was angry
    memset(&tmpnode, 0, sizeof(Inode));

    for (i=0; i<inodeArr.capacity; i++)
    {
        tmpnode.inode = inodeArr.inode[i].inode;
        //write 32 bytes, either including or excluding this null character
        strncpy(tmpnode.name, inodeArr.inode[i].name, 32);
        tmpnode.parentInode = inodeArr.inode[i].parentInode;
        tmpnode.type = inodeArr.inode[i].type;
        fwrite(&tmpnode, sizeof(Inode), 1, fp);
    }
    fclose(fp);
}

//change the working directory by updating global variable
void changeDirectory(const char *name) {
    int currentNodes = inodeArr.capacity;
    int i;
    int check = -1; 
    for (i=0; i<currentNodes; i++)
    {
        if (strcmp(inodeArr.inode[i].name, name) == 0)
        {
            if (inodeArr.inode[i].parentInode == currentDirInode)
            {
                if (inodeArr.inode[i].type == 'd')
                {
                    currentDirInode = inodeArr.inode[i].inode;
                    check = 0;
                }
            }
        }
    }
    if (strcmp(name, ".") == 0)
    {
        //do nothing
        check = 0;
    }
    else if (strcmp(name, "..") == 0)
    {
        //if root do nothing
        if (currentDirInode == inodeArr.inode[0].inode)
        {
            check = 0;
        }
        //has meaningful change
        else
        {
            currentDirInode = inodeArr.inode[currentDirInode].parentInode;
            check = 0;
        }
    }
    if (check == -1)
    {
        //printf(name);
        printf("Directory not found\n");
    }
    return;
}

//list the contents of the current directory
void listContents() {
    int currentNodes = inodeArr.capacity;
    int i;
    for (i=0; i<currentNodes; i++)
    {
        //print inodes in current dir
        if (inodeArr.inode[i].parentInode == currentDirInode)
        {
            printf("Inode: %u, ", inodeArr.inode[i].inode);
            printf("Type: %c, ", inodeArr.inode[i].type);
            printf("Name: %s\n", inodeArr.inode[i].name);
        }
    }
}

//create directory, insert inode, etc
void createDirectory(const char *name) {
    //do checks on existing nodes, make sure it can be made
    int currentNodes = inodeArr.capacity;
    int i;
    for (i=0; i<currentNodes; i++)
    {
        if ((strcmp(inodeArr.inode[i].name, name) == 0) && 
        (inodeArr.inode[i].parentInode == currentDirInode))
        {
            printf("File/Directory with that name already exists!\n");
            return;
        }
        else if (inodeArr.capacity >= 1024)
        {
            printf("Inode max limit of 1024 reached");
            return;
        }
    }
    //insert the node if no other complications.
    Inode inode;
    inode.inode = inodeArr.capacity;
    strcpy(inode.name, name);
    inode.parentInode = currentDirInode;
    inode.type = 'd';
    addInode(&inodeArr);
    insertInode(&inodeArr, inode);
    char *inodeStr = uint32_to_str(inode.inode);
    if (inodeStr == NULL)
    {
        perror("Malloc Error");
        exit(1);
    }
    FILE *fp = fopen(inodeStr, "w");
    if (fp == NULL) 
    {
        perror("Error opening file");
        exit(1);
    }
    free(inodeStr);
    fprintf(fp, "%d .\n%d ..", inode.inode, inode.parentInode);
    fclose(fp);
}

//Create a file, insert inode, etc.
void createFile(const char *name) {
    //do checks on existing nodes, make sure it can be made
    int currentNodes = inodeArr.capacity;
    int i;
    for (i=0; i<currentNodes; i++)
    {
        if ((strcmp(inodeArr.inode[i].name, name) == 0) && 
        (inodeArr.inode[i].parentInode == currentDirInode))
        {
            printf("File/Directory with that name already exists!\n");
            return;
        }
        else if (inodeArr.capacity >= 1024)
        {
            printf("Inode max limit of 1024 reached");
            return;
        }
    }
    //insert the node if no other complications.
    Inode inode;
    inode.inode = inodeArr.capacity;
    strcpy(inode.name, name);
    inode.parentInode = currentDirInode;
    inode.type = 'f';
    addInode(&inodeArr);
    insertInode(&inodeArr, inode);
    char *inodeStr = uint32_to_str(inode.inode);
    if (inodeStr == NULL)
    {
        perror("Malloc Error");
        exit(1);
    }
    FILE *fp = fopen(inodeStr, "w");
    if (fp == NULL) 
    {
        perror("Error opening file");
        exit(1);
    }
    free(inodeStr);
    fprintf(fp, "%s", inode.name);
    fclose(fp);
}

//make function to splice 32 chars after commands, return that 32 char str
char *getName(int start, char*str)
{
    int strlength = strlen(str);
    //allocate 32 bytes + 1 for possible NULL
    char* retStr = (char*)malloc(ARG_MAX * sizeof(char) + 1);
    if (retStr == NULL)
    {
        perror("Malloc Error");
        exit(1);
    }
    int i;
    for (i=0; i<(strlength-start); i++)
    {
        if ((i == (ARG_MAX)))
        {
            //means the name is too long
            return NULL;
        }
        retStr[i] = str[i+start-1];
    }

    retStr[i] = '\0'; // null terminate. pretty much for testing.
    return retStr;
}

//imported from asgn description
char *uint32_to_str(uint32_t i) {
int length = snprintf(NULL, 0, "%u", i);
if (length < 0) return NULL;
char* str = (char*) malloc(sizeof(char) * (length + 1));
if (str == NULL) return NULL;
snprintf(str, length + 1, "%u", i);
return str;
}

//Behavior of empty? (ls prints nothing, but seems to have 0 inode)
//...(?) just means empty has not been set up correctly maybe
// when touching, if no file, then can use empty as root i guess