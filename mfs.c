#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#define _GNU_SOURCE
#define MAX_COMMAND_SIZE 255
#define MAX_NUM_ARGUMENTS 5
#define WHITESPACE " \t\n"

//structure used for the 16 clusters within the directories
char BS_OEMName[8];
int16_t BPB_BytsPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATs;
int16_t BPB_RootEntCnt;
char BS_VolLab[11];
int32_t BPB_FATSz32;
int32_t BPB_RootClus;

int32_t RootDirSectors = 0;
int32_t FirstDataSector = 0;
int32_t FirstSectorofCluster = 0;

struct __attribute__((__packed__)) DirectoryEntry
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];


int LBAToOffset(int32_t sector)
{
  return ((sector - 2) * BPB_BytsPerSec)+(BPB_BytsPerSec * BPB_RsvdSecCnt) +
    (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec);
}

int16_t NextLB(uint32_t sector, FILE * fp)
{
  uint32_t FATAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt) + (sector * 4);
  int16_t val;
  fseek(fp, FATAddress, SEEK_SET);
  fread(&val,2,1,fp);
  return val;
}

FILE * open(char * token[], int token_count)
{
  if( access(token[1], F_OK ) != -1 )
  {
    FILE * fp = fopen(token[1], "r");
    fseek(fp,3,SEEK_SET);
    fread(&BS_OEMName,1,3,fp);

    fseek(fp,11,SEEK_SET);
    fread(&BPB_BytsPerSec,1,2,fp);

    fseek(fp,13,SEEK_SET);
    fread(&BPB_SecPerClus,1,1,fp);

    fseek(fp,14,SEEK_SET);
    fread(&BPB_RsvdSecCnt,1,2,fp);

    fseek(fp,16,SEEK_SET);
    fread(&BPB_NumFATs,1,1,fp);

    fseek(fp,17,SEEK_SET);
    fread(&BPB_RootEntCnt,1,2,fp);

    fseek(fp,71,SEEK_SET);
    fread(&BS_VolLab,1,11,fp);

    fseek(fp,36,SEEK_SET);
    fread(&BPB_FATSz32,1,4,fp);

    fseek(fp,44,SEEK_SET);
    fread(&BPB_RootClus,1,4,fp);

    int clusters;
    int Offset = ((BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec)+ (BPB_RsvdSecCnt *BPB_BytsPerSec));
    fseek(fp,Offset,SEEK_SET);
    for(clusters = 0; clusters < 16; clusters++)
    {
      fread(&dir[clusters],1,32,fp);
    }
    return fp;
  }else
  {
    printf("Error: File system image not found.\n\n");
    return NULL;
  }
}


void info()
{
  printf("BPB_BytsPerSec : %d\nBPB_BytesPerSec : %x\n\nBPB_SecPerClus : %d\n"
         "BPB_SecPerClus : %x\n\nBPB_RsvdSecCnt: %d\nBPB_RsvdSecCnt: %x\n\n"
         "BPB_NumFATs: %d\nBPB_NumFATs: %x\n\nBPB_FATSz32: %d\nBPB_FATSz32: %x\n\n",
         BPB_BytsPerSec,BPB_BytsPerSec,BPB_SecPerClus,BPB_SecPerClus,BPB_RsvdSecCnt,
         BPB_RsvdSecCnt,BPB_NumFATs,BPB_NumFATs,BPB_FATSz32,BPB_FATSz32);
}

void printDir(FILE * fp,int Offset)
{
  int clusters;
  for(clusters = 0; clusters < 16; clusters++)
  {
    if((dir[clusters].DIR_Attr == 1 || dir[clusters].DIR_Attr == 48 || 
        dir[clusters].DIR_Attr == 16 || dir[clusters].DIR_Attr ==32) &&
        dir[clusters].DIR_Name[0] != (char)0xe5)
    {
      printf("%.11s\n",dir[clusters].DIR_Name);
    }
  }
}


//cd .. works with deeper directories, but could not get it to work
//going back to the root directory
int cd(FILE * fp,uint16_t DIR_FirstClusterLow)
{
  int clusters;

  int clusterOffset = LBAToOffset(DIR_FirstClusterLow);

  fseek(fp,clusterOffset,SEEK_SET);

  for(clusters = 0; clusters < 16; clusters++)
  {
    fread(&dir[clusters],1,32,fp);
  }
  return clusterOffset;
}

void read(FILE * fp,char *filename,int position,int No_byte, int rootOffset)
{
  int k;
  // Changing lowercase alphabet to uppercase ones
  for (k=0;k<strlen(filename);k++)
  {
    filename[k]=toupper(filename[k]);
  }
  // comparing input filename with existing files in FAT32
  int clusters=0;
  fseek(fp,rootOffset,SEEK_SET);
  for(clusters = 0; clusters < 16; clusters++)
  {
    fread(&dir[clusters],1,32,fp);
    if(strncmp(dir[clusters].DIR_Name,filename,3)==0)
    {
      break;
    }
  }

  if (clusters==16)
  {
    printf("No file with this name is here.\n");
  }else
  {
  // Going to specific location in file	
    int first_cluster = dir[clusters].DIR_FirstClusterLow;
    int k;
    if (position>dir[clusters].DIR_FileSize)
    {
      printf("Position is more than file size.\n");
    }else
    {
      int16_t next = first_cluster;
      for(k=0;k<((position/512));k++)
      {
        next = NextLB(next,fp);
      }

      int cluster_byte=LBAToOffset(next);
      int cluster_offset = position-(k*512);
      int counter=cluster_byte+cluster_offset;

      // Printing bytes
      char result[No_byte];
      int l=0;
      for (l=0;l<No_byte;l++)
      {
        fseek(fp,counter,SEEK_SET);
        fread(&result[l],1,1,fp);
        printf("%x ",result[l]);
        counter++;
      }
      printf("\n");
    }
  }
}

void stat(FILE * fp, char * filename, int rootOffset)
{
  // Changing lowercase alphabet to uppercase ones
  int k;
  for (k=0;k<strlen(filename);k++)
  {
    filename[k]=toupper(filename[k]);
  }
  fseek(fp,rootOffset,SEEK_SET);

  // comparing input filename with existing files in FAT32
  // and printing Attribute, first cluster number and file size.

  int clusters;
  for(clusters = 0; clusters < 16; clusters++)
  {
    fread(&dir[clusters],1,32,fp);
    if(strncmp(dir[clusters].DIR_Name,filename,3)==0)
    {
      break;
    }
  }
  if (clusters==16)
  {
    printf("Error:File not found.\n");
  }
  else
  {
    printf(" Attr is: %d\n",dir[clusters].DIR_Attr);
    printf(" Starting Cluster Number is:%d\n",dir[clusters].DIR_FirstClusterLow);
    if (dir[clusters].DIR_Attr!=16)
    {
      printf("File size is:%d\n",dir[clusters].DIR_FileSize);
    }
    else
    {
      printf("File size is:0\n");
    }
  } 
}

int get(FILE * fp,char * token[])
{
  int i;
  int x;
  int sizeOfFile;
  uint16_t firstCluster;
  char dirName[12];
  char extension[5];
  extension[0] = '.';
  extension[4] = '\0';
  int realNameSize = 0;
  int entry;

  char name[12] = "           ";
  name[11] = '\0';
  int k;
  int v = 0;
  for (k=0;k<strlen(token[1]);k++)
  {
    if(token[1][k] == '.')
    {
      v = k+1;
      break;
    }
    else
    {
      name[k]=toupper(token[1][k]);
    }
  }
  if(v != 0)
  {
    for (k=0;k < 3;k++)
    {
      name[8+k]=toupper(token[1][v]);
      v++;
    }
  }

  for(i = 0; i < 16; i++)
  {
    strncpy(dirName,dir[i].DIR_Name,12);
    dirName[11]='\0';
    if(strstr( dirName,name) != NULL)
    {
      sizeOfFile = dir[i].DIR_FileSize;
      firstCluster = dir[i].DIR_FirstClusterLow;
      entry = i;
      break;
    }

    if(i == 15)
    {
      printf("Error: File not found\n");
      return 0;
    }
  }

  char buffer[sizeOfFile];
  int firstClustOffset = LBAToOffset(firstCluster);
  fseek(fp,firstClustOffset,SEEK_SET);
  for(i = 0; i < 11;i++)
  {
    if(dir[entry].DIR_Name[i] != ' ')
    {
      realNameSize++;
    }
    if(i < 3)
    {
      extension[i+1] = dir[entry].DIR_Name[8+i];
    }
    if(i == 10)
    {
      realNameSize+=2;
    }
  }

  char newName[realNameSize];
  newName[realNameSize-1]='\0';
  x = 0;

  for(i = 0; i < realNameSize; i++)
  {
    if(i < (realNameSize-5))
    {
      newName[i] = dir[entry].DIR_Name[i];
    }
    else
    {
      newName[i] = extension[x];
      x++;
    }
  }

  printf("%s\n",newName);
  FILE * newFile = fopen(newName, "w+");

  for(i = 0; i < sizeOfFile; i++)
  {
    fread(&buffer[i],1,1,fp);
    firstClustOffset++;
  }

  fwrite(&buffer,1,sizeOfFile,newFile);
  fclose(newFile);
  //may remove this
  return 0;
}

int main()
{
  // getting user input
  char *folderName[10];
  int rootOffset;
  int i,h,cd_num=0;
  FILE * fp = NULL;
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    printf ("msh>");
    for (h=0;h<cd_num;h++)
    {
      if (folderName[h]!=NULL & strcmp(".",folderName[h])!= 0)
      {
        printf ("%s>",folderName[h]);
      }
    }

    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    char *token[MAX_NUM_ARGUMENTS];
    int   token_count = 0;

    char *arg_ptr;
    char *working_str  = strdup( cmd_str );
    char *working_root = working_str;

    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
      token_count++;
    }

    //function implementation start below

    if(token[0] != NULL)
    {
      if(strcmp("open",token[0]) == 0)
      {
        fp = open(token,token_count);
        rootOffset = ((BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec)+(BPB_RsvdSecCnt *BPB_BytsPerSec));
      }
      else if(strcmp("close",token[0]) == 0 && fp != NULL)
      {
        fclose(fp);
        fp = NULL;
      }
      else if(strcmp("info",token[0]) == 0 && fp != NULL)
      {
        info();
      }
      else if(strcmp("ls",token[0]) == 0 && fp != NULL)
      {
        printDir(fp,rootOffset);
      }
      else if(strcmp("read",token[0]) == 0 && fp != NULL)
      {
        char *name=token[1];
        int position=atoi(token[2]);
        int No_byte=atoi(token[3]);
        read(fp,name,position,No_byte,rootOffset);
      }
      else if(strcmp("volume",token[0]) == 0 && fp != NULL)
      {
        char *BS_VolLab[11];
        fseek(fp,71,SEEK_SET);
        fread(&BS_VolLab,1,11,fp);

        if (BS_VolLab!=NULL)
        {
          printf("Volume name of the file is:%d \n",BS_VolLab,BS_VolLab);
        }
        else
        {
          printf("Error: Volume name not found. \n");
        }
      }
      else if(strcmp("stat",token[0]) == 0 && fp != NULL)
      {
        char *filename=token[1];
        stat(fp,filename,rootOffset);
      }
      else if(strcmp("cd",token[0]) == 0 && fp != NULL)
      {
        for(i = 0; i < 16; i++)
        {
          char dirName[12];
          strncpy(dirName,"000000000000",12);
          strncpy(dirName,dir[i].DIR_Name,11);
          if(strstr( dirName,token[1]) != NULL)
          {
            rootOffset = cd(fp,dir[i].DIR_FirstClusterLow);

            // Below part is for mfs prompt to show the current directory name:
            if (strcmp(token[1],"..")!=0)
            {
              folderName[cd_num]=token[1];
              cd_num += 1;
            }
            else
            {
              cd_num -= 1;
            }
            break;
          }

          if(i == 15)
          {
            printf("File not found.\n");
          }
        }
      }
      else if(strcmp("get",token[0]) == 0 && fp != NULL)
      {
        get(fp,token);
      }
      else if(token[0] != NULL && fp == NULL)
      {
        printf("Error: File system image must be opened first.\n");
      }
    }

    free( working_root );
  }
  return 0;
}

