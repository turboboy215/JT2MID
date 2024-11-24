/*Jeroen Tel (GB/GBC) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt, * cfg;
long bank;
long patTable;
long seqTable;
long speedTable;
int i, j;
char outfile[1000000];
int songNum;
int numSongs;
int patNums[4];
long songPtrs[4];
long base;
int numSeqs;
int songTrans;
unsigned static char* romData;
unsigned long seqList[500];
int curIns;

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptrList[4], int songTrans);
void seqs2txt();

char string1[100];
char string2[100];
char checkStrings[11][100] = { "bank=", "numsongs=", "audiotab=", "steptab=",  "base=", "trans=", "chn1=", "chn2=", "chn3=", "chn4=", "speedtab=" };

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}


int main(int args, char* argv[])
{
	printf("Jeroen Tel (GB/GBC) to TXT converter\n");
	if (args != 3)
	{
		printf("Usage: JT2TXT <rom> <config>\n");
		return -1;
	}
	else
	{
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open ROM file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			if ((cfg = fopen(argv[2], "r")) == NULL)
			{
				printf("ERROR: Unable to open configuration file %s!\n", argv[1]);
				exit(1);
			}
			else
			{
				/*Get the bank value*/
				fgets(string1, 6, cfg);

				if (memcmp(string1, checkStrings[0], 1))
				{
					printf("ERROR: Invalid CFG data!\n");
					exit(1);

				}
				fgets(string1, 3, cfg);
				bank = strtol(string1, NULL, 16);
				printf("Bank: %01X\n", bank);

				/*Skip new line*/
				fgets(string1, 2, cfg);
				/*Get the total number of songs*/
				fgets(string1, 10, cfg);

				if (memcmp(string1, checkStrings[1], 1))
				{
					printf("ERROR: Invalid CFG data!\n");
					exit(1);

				}
				fgets(string1, 3, cfg);
				numSongs = strtod(string1, NULL);
				printf("Total # of songs: %i\n", numSongs);

				/*Skip new line*/
				fgets(string1, 2, cfg);
				/*Get the "audiotab" (pattern data pointer)*/
				fgets(string1, 10, cfg);

				if (memcmp(string1, checkStrings[2], 1))
				{
					printf("ERROR: Invalid CFG data!\n");
					exit(1);

				}
				fgets(string1, 5, cfg);
				patTable = strtol(string1, NULL, 16);
				printf("Pattern table: 0x%04X\n", patTable);

				/*Skip new line*/
				fgets(string1, 2, cfg);
				/*Get the "steptab" (sequence pointer)*/
				fgets(string1, 9, cfg);

				if (memcmp(string1, checkStrings[3], 1))
				{
					printf("ERROR: Invalid CFG data!\n");
					exit(1);

				}
				fgets(string1, 5, cfg);
				seqTable = strtol(string1, NULL, 16);
				printf("Sequence table: 0x%04X\n", seqTable);

				/*Skip new line*/
				fgets(string1, 2, cfg);
				/*Get the speed pointer*/
				fgets(string1, 10, cfg);

				if (memcmp(string1, checkStrings[10], 1))
				{
					printf("ERROR: Invalid CFG data!\n");
					exit(1);

				}
				fgets(string1, 5, cfg);
				speedTable = strtol(string1, NULL, 16);
				printf("Speed table: 0x%04X\n", speedTable);

				/*Skip new line*/
				fgets(string1, 2, cfg);
				/*Get "base" value*/
				fgets(string1, 6, cfg);

				if (memcmp(string1, checkStrings[4], 1))
				{
					printf("ERROR: Invalid CFG data!\n");
					exit(1);

				}
				fgets(string1, 5, cfg);
				base = strtol(string1, NULL, 16);
				printf("Base: 0x%04X\n", base);

				/*Copy the ROM data*/
				fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
				romData = (unsigned char*)malloc(bankSize);
				fread(romData, 1, bankSize, rom);
				fclose(rom);

				/*Get the pointers for each song and convert*/
				for (songNum = 1; songNum <= numSongs; songNum++)
				{
					/*Skip new line*/
					fgets(string1, 2, cfg);
					/*Skip the first line*/
					fgets(string1, 10, cfg);

					/*Get transpose value*/
					fgets(string1, 7, cfg);
					if (memcmp(string1, checkStrings[5], 1))
					{
						printf("ERROR: Invalid CFG data!\n");
						exit(1);
					}
					fgets(string1, 3, cfg);

					songTrans = (signed char)strtol(string1, NULL, 16);

					for (i = 0; i < 4; i++)
					{
						/*Skip new line*/
						fgets(string1, 2, cfg);
						/*Skip "chnX="*/
						fgets(string1, 6, cfg);

						/*Get the pattern numbers*/
						fgets(string1, 3, cfg);

						patNums[i] = strtol(string1, NULL, 16);
					}

					j = patTable - base;
					for (i = 0; i < 4; i++)
					{
						songPtrs[i] = ReadLE16(&romData[j + ((patNums[i]) * 2)]);
						printf("Song %i channel %i pattern number: %01X\n", songNum, (i + 1), patNums[i]);
						printf("Song %i channel %i pattern address: 0x%04X\n", songNum, (i + 1), songPtrs[i]);
					}
					song2txt(songNum, songPtrs, songTrans);
				}

				seqs2txt();

				printf("The operation was successfully completed!\n");
				exit(0);
			}
		}
	}
}

void song2txt(int songNum, long ptrList[4], int songTrans)
{
	int patPos = 0;
	int seqNum = 0;
	int songEnd = 0;
	int curTrack = 0;
	int command[2] = { 0, 0 };
	int transpose = 0;
	int speedVals[2] = { 0, 0 };

	sprintf(outfile, "song%d.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.txt!\n", songNum);
		exit(2);
	}
	else
	{
		for (curTrack == 0; curTrack < 4; curTrack++)
		{
			fprintf(txt, "Channel %i pattern: 0x%04X\n", (curTrack + 1), ptrList[curTrack]);
			patPos = ptrList[curTrack] - base;
			songEnd = 0;

			while (songEnd == 0)
			{
				command[0] = romData[patPos];
				command[1] = romData[patPos + 1];

				if (command[0] < 0x70)
				{
					seqNum = command[0];
					fprintf(txt, "Play sequence: %01X\n", seqNum);
					patPos++;
				}

				else if (command[0] >= 0x70 && command[0] < 0x80)
				{
					transpose = command[0] - 0x70;
					fprintf(txt, "Sequence effect: %01X\n", transpose);
					patPos++;
				}

				else if (command[0] >= 0x80 && command[0] < 0xFE)
				{
					transpose = command[0] - 0x80;
					fprintf(txt, "Transpose: %i\n", transpose);
					patPos++;
				}

				else if (command[0] == 0xFE)
				{
					fprintf(txt, "End pattern without loop\n\n");
					songEnd = 1;
				}

				else if (command[0] == 0xFF)
				{
					fprintf(txt, "Loop pattern back to pattern %01X\n\n", command[1]);
					songEnd = 1;
				}
			}
		}
	}
}

void seqs2txt()
{
	int seqPos = 0;
	int seqNum = 0;
	int curSeq = 0;
	int seqEnd = 0;
	int curNote = 0;
	int curNoteLen = 0;
	int command[3] = { 0, 0, 0};


	sprintf(outfile, "seqs.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file seqs.txt!\n");
		exit(2);
	}
	else
	{
		for (seqNum == 0; seqNum < 128; seqNum++)
		{
			curSeq = ReadLE16(&romData[(seqTable - base) + (seqNum * 2)]);
			fprintf(txt, "Sequence %i (0x%04X):\n", seqNum, curSeq);
			seqEnd = 0;
			seqPos = curSeq - base;

			while (seqEnd == 0)
			{
				command[0] = romData[seqPos];
				command[1] = romData[seqPos + 1];
				command[2] = romData[seqPos + 2];

				if (command[0] < 0x60)
				{
					curNote = command[0];
					fprintf(txt, "Play note: %01X\n", curNote);
					seqPos++;
				}

				else if (command[0] >= 0x60 && command[0] < 0x80)
				{
					fprintf(txt, "Set arpeggio: %01X\n", command[0]);
					seqPos++;
				}

				else if (command[0] >= 0x80 && command[0] < 0xC0)
				{
					curNoteLen = command[0] - 0x80;
					fprintf(txt, "Change note length: %i\n", curNoteLen);
					seqPos++;
				}

				else if (command[0] >= 0xC0 && command[0] < 0xE0)
				{
					curIns = command[0] - 0xC0;
					fprintf(txt, "Current instrument: %i\n", curIns);
					seqPos++;
				}

				else if (command[0] >= 0xE0 && command[0] < 0xFD)
				{
					curNoteLen = command[0] - 0xE0;
					fprintf(txt, "Delay: %i\n", curNoteLen);
					seqPos++;
				}

				else if (command[0] == 0xFD)
				{
					fprintf(txt, "Portamento: %i, %i\n", command[1], command[2]);
					seqPos += 3;
				}

				else if (command[0] == 0xFE)
				{
					fprintf(txt, "Stop sequence\n");
					seqEnd = 1;
				}

				else if (command[0] == 0xFF)
				{
					fprintf(txt, "End of sequence\n\n");
					seqEnd = 1;
				}
				
				else
				{
					fprintf(txt, "Unknown command: %01X\n", command[0]);
					seqPos++;
				}
			}
		}

	}


}