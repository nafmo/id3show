/* id3show.c
 * $Id$
 * © 1998 Peter Karlsson
 *
 * This program is released under the GNU General Public License.
 */

#include <stdio.h>
#include <string.h>

typedef struct
{
	char			tag[3],
					songname[30],
					artist[30],
					album[30],
					year[4],
					comment[30];
	unsigned char	genre;
} id3_t;

char *genre[]= {"Blues", "Classic Rock", "Country", "Dance", "Disco",
               "Funk", "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age",
               "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
               "Techno", "Industrial", "Alternative", "Ska", "Death Metal",
               "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
               "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical",
               "Instrumental", "Acid", "House", "Game", "Sound Clip",
               "Gospel", "Noise", "AlternRock", "Bass", "Soul", "Punk",
               "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
               "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
               "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock",
               "Comedy", "Cult", "Gangsta", "Top 40", "Christian Rap",
               "Pop/Funk", "Jungle", "Native American", "Cabaret", "New Wave",
               "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal",
               "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
               "Rock & Roll", "Hard Rock", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               ""};

void showid3(const char *);
void strip(char *);

int main(int argc, char *argv[])
{
	int i;

	// Check if we need help
	if (argc < 2 || !strcmp(argv[1], "--help"))
	{
		printf("Shows ID3 tags\nUsage: %s filenames\n", argv[0]);
	}

	// Show ID3 tags of all files
	for (i = 1; i < argc; i ++)
		showid3(argv[i]);
}

// showid3:
//  shows ID3 tags
void showid3(const char *filename)
{
	FILE	*fp;
	id3_t	tag;

	// Open file and retrieve ID3
	fp = fopen(filename, "rb");
	if (!fp) return;
	fseek(fp, -sizeof(tag), SEEK_END);
	fread(&tag, sizeof(tag), 1, fp);
	fclose(fp);
	
	// Check for integrity, and print info if it's okay
	if ('T' == tag.tag[0] && 'A' == tag.tag[1] && 'G' == tag.tag[2])
	{
		char	tmp[32] = {0};

		puts(filename);
		strncpy(tmp, tag.artist, 30);
		strip(tmp);
		if (!tmp[0]) strcpy(tmp, "Unknown artist");
		printf(" %s - ", tmp);
		
		strncpy(tmp, tag.songname, 30);
		strip(tmp);
		if (!tmp[0]) strcpy(tmp, "Unknown title");
		printf("%s\n", tmp);

		strncpy(tmp, tag.album, 30);
		strip(tmp);
		if (!tmp[0]) strcpy(tmp, "Unknown album");
		printf(" [%s", tmp);
		
		strncpy(tmp, tag.year, 4);
		tmp[4] = 0;
		strip(tmp);
		if (tmp[0])
			printf(" %s", tmp);
		fputc(']', stdout);
		fputc('\n', stdout);
		
		if (tag.genre < 80)
			printf(" %s\n", genre[tag.genre]);
		else
			puts(" Unknown genre");

		fputc('\n', stdout);
	}
}

// strip:
//  Strip excess spaces from strings
void strip(char *s)
{
	while (' ' == s[strlen(s) - 1])
		s[strlen(s) - 1] = 0;
}
