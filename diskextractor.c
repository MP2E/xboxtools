/*

haleyjd: test tool to extract data from a .disk file

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;

//
// .disk directory structure
//
typedef struct diskfileinfo_s
{
	char filename[64];
	unsigned int offset;
	unsigned int length;
} diskfileinfo_t;

unsigned int numfiles;
unsigned int totalsize;
diskfileinfo_t *directory;

unsigned int FixBEUInt(unsigned int x)
{
	return (((byte *) &x)[0] << 24) +
		(((byte *) &x)[1] << 16) +
		(((byte *) &x)[2] <<  8) +
		((byte *) &x)[3];
}

//
// ReadNumFiles
// 
// Read number of files in the .disk file
//
void ReadNumFiles(FILE *diskfile)
{
	fread(&numfiles, 4, 1, diskfile);

	numfiles = FixBEUInt(numfiles);
}

//
// ReadDirectory
// 
// Read directory entries
//
void ReadDirectory(FILE *diskfile)
{
	unsigned int i;

	directory = calloc(numfiles, sizeof(*directory));

	for(i = 0; i < numfiles; i++)
	{
		fread(&directory[i].filename, 1, 64, diskfile); // read name
		fread(&directory[i].offset,   4,  1, diskfile); // read offset
		fread(&directory[i].length,   4,  1, diskfile); // read length

		// fix offset and length
		directory[i].offset = FixBEUInt(directory[i].offset);
		directory[i].length = FixBEUInt(directory[i].length);
	}
}

//
// ReadTotalSize
//
// Read the total size of the file, minus the header length.
//
void ReadTotalSize(FILE *diskfile)
{
	fread(&totalsize, 4, 1, diskfile);

	totalsize = FixBEUInt(totalsize);
}

//
// DumpFiles
//
// Dumps all the files to disk.
//
void DumpFiles(FILE *diskfile)
{
	unsigned int i;
	unsigned int headersize = 8 + 72 * numfiles;

	for(i = 0; i < numfiles; i++)
	{
		byte *tempbuf;
		diskfileinfo_t *info = &directory[i];
		char *name = NULL;

		// malloc a buffer
		tempbuf = calloc(1, info->length);

		// seek to beginning of input
		fseek(diskfile, info->offset + headersize, SEEK_SET);

		// read input file
		fread(tempbuf, 1, info->length, diskfile);

		// get output file name
		name = strrchr(info->filename, '\\');

		if(name)
		{
			FILE *outfile;
			++name;

			if((outfile = fopen(name, "wb")))
			{
				printf("Dumping file %s\n", name);
				fwrite(tempbuf, 1, info->length, outfile);
				fclose(outfile);
			}
		}

		free(tempbuf);
	}
}

//
// main
//
int main(int argc, char **argv)
{
	FILE *archive;

	if(argc < 2)
		return 0;

	// open the archive
	if(!(archive = fopen(argv[1], "rb")))
	{
		puts("couldn't open input file\n");
		return 0;
	}

	// read number of files, directory, and total size
	ReadNumFiles(archive);
	ReadDirectory(archive);
	ReadTotalSize(archive);

	// dump all files
	DumpFiles(archive);

	// close archive
	fclose(archive);

	return 0;
}
