#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//structure used for the 16 clusters within the directories

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

int main()
{
	int i;

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

	FILE * fp = fopen("fat32.img", "r");
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

	printf("OEMName: %s\nBPB_BytsPerSec: %d\nBPB_SecPerClus: %d\nBPB_RsvdSecCnt: %d\nBPB_NumFATs: %d\nBPB_RootEntCnt: %d\nBS_VolLab: %s\nBPB_FATSz32: %d\nBPB_RootClus: %d\n",BS_OEMName,BPB_BytsPerSec,BPB_SecPerClus,BPB_RsvdSecCnt,BPB_NumFATs,BPB_RootEntCnt,BS_VolLab,BPB_FATSz32,BPB_RootClus);
	/*
	 * very first cluster holds the root directory
	 * data formatted differently. holds 16, 32byte records that tell you the files that are in the
	 * directory and attributes they have and the clusters of where they are at(where to go to
	 *  find that file) and the file size.
	 *
	 *
	 *  jump into the root dir and read sixteen clusters 
	 *  if you read the first char of a file and see one of the special chars,
	 *  do something
	*/

//we are getting to the root directory and populating it here
//
	fseek(fp,((BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) +(BPB_RsvdSecCnt *
BPB_BytsPerSec)),SEEK_SET);
	for(i = 0; i < 16; i++)
	{
		fread(&dir[i],1,32,fp);
		if(dir[i].DIR_Name[0] != (char)0xe5)
		{
		printf("DIR_Name:%.11s\n",dir[i].DIR_Name);
		}
	}


return 0;
}
