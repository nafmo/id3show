/* id3show.c
 * © 1998-2010 Peter Krefting <peter@softwolves.pp.se>
 *
 * This program is released under the GNU General Public License
 * version 2.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#define FALSE	0
#define TRUE	1

typedef struct
{
	char			tag[3],
					songname[30],
					artist[30],
					album[30],
					year[4];
	union
	{
		char		comment[30];
		struct
		{
			char	comment[28];
			char	null;
			unsigned char tracknum;
		} n;
	} 				v;
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
               "Electronic", "Pop-Folk", "Eurodance", "Dream",
               "Southern Rock", "Comedy", "Cult", "Gangsta", "Top 40",
               "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
               "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer",
               "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz", "Polka",
               "Retro", "Musical", "Rock & Roll", "Hard Rock", "Folk",
               "Folk-Rock", "National folk", "Swing", "Fast fusion",
               "Bebob", "Latin", "Revival", "Celtic", "Bluegrass",
               "Avantgarde", "Gothic Rock", "Progressive Rock",
               "Psychadelic Rock", "Symphonic Rock", "Slow Rock", "Big band",
               "Chorus", "Easy listening", "Acoustic", "Humour", "Speech",
               "Chanson", "Opera", "Chamber music", "Sonata", "Symphony",
               "Booty bass", "Primus", "Porn groove", "Satire", "Slow jam",
               "Club", "Tango", "Samba", "Folklore", "Ballad",
               "Power ballad", "Rhythmic soul", "Freestyle", "Duet",
               "Punk Rock", "Drum solo", "A Capella", "Euro-House",
               "Dance hall", "Goa", "Drum & Bass", "Club-House", "Hardcore",
               "Terror", "Indie", "BritPop", "Negerpunk", "Polsk Punk",
               "Beat", "Christian Gangsta Rap", "Heavy Metal", "Black Metal",
               "Crossover", "Contemporary Christian", "Christian Rock",
               "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
               "Synthpop", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
               "", "", "", "", "", ""};

void showid3(const char *, int, int);
void showid3v1(FILE *, const char *, int, int);
void strip(char *);

int main(int argc, char *argv[])
{
	int	i, c, isshort, islist, ismv;

	// Check if we need help
	if (argc < 2 || !strcmp(argv[1], "--help"))
	{
		printf("Shows ID3 tags\n"
		       "Usage: %s [-s|-m] filenames\n"
		       "       %s -g\n\n", argv[0], argv[0]);
		puts("  -s   Short list format");
		puts("  -m   Create renaming list");
		puts("  -g   List genres");
		return 0;
	}

	// Handle arguments
	isshort = FALSE;
	islist = FALSE;
	ismv = FALSE;

	while (EOF != (c = getopt(argc, argv, "sgm")))
	{
		switch (c)
		{
		case 's':
			isshort = TRUE;
			break;
		case 'm':
			ismv = TRUE;
			break;
		case 'g':
			islist = TRUE;
			break;
		}
	}

	if (islist)
	{
		// List all ID3 genres
		for (i = 0; i < 255; i ++)
			if (genre[i][0])
				printf("%3d: %s\n", i, genre[i]);
	}
	else
	{
		// Show ID3 tags of all files
		for (i = optind; i < argc; i ++)
			showid3(argv[i], isshort, ismv);
	}
}

// showid3:
//  shows ID3 tags
void showid3(const char *filename, int isshort, int ismv)
{
	FILE	*fp;

	// Open file and retrieve ID3
	fp = fopen(filename, "rb");

	showid3v1(fp, filename, isshort, ismv);
	fclose(fp);
}

// showid3v1:
//  shows an ID3v1 tag
void showid3v1(FILE *fp, const char *filename, int isshort, int ismv)
{
	id3_t	tag;
	int		i;

	// Retrieve ID3v1 from the end of the file
	if (!fp) return;
	if (-1 == fseek(fp, -sizeof(tag), SEEK_END))
	{
		return;
	}
	if (0 == fread(&tag, sizeof(tag), 1, fp))
	{
		return;
	}

	// Check for integrity, and print info if it's okay
	if ('T' == tag.tag[0] && 'A' == tag.tag[1] && 'G' == tag.tag[2])
	{
		char	tmp[32] = {0};

		if (ismv)
		{
			int j;

			// mv output format:
			// "mv filename num-artist-title.mp3"
			strncpy(tmp, tag.artist, 30);
			strip(tmp);
			j = 0;
			for (i = 0; i < strlen(tmp); i ++)
			{
				if ('å' == tmp[i] || 'Å' == tmp[i] ||
				    'ä' == tmp[i] || 'Ä' == tmp[i])
				    tmp[j ++] = 'a';
				else if ('ö' == tmp[i] || 'Ö' == tmp[i])
				    tmp[j ++] = 'o';
				else if ('é' == tmp[i] || 'É' == tmp[i])
				    tmp[j ++] = 'e';
				else if (isalpha(tmp[i]))
					tmp[j ++] = tolower(tmp[i]);
				else if (isdigit(tmp[i]))
					tmp[j ++] = tmp[i];
				else if (' ' == tmp[i])
					tmp[j ++] = '_';
			}
			tmp[j] = 0;
			printf("mv -i \"%s\" \"", filename);
			if (0 == tag.v.n.null && tag.v.n.tracknum)
				printf("%02u-", tag.v.n.tracknum);
			else
			{
				int tracknum;
				if (1 == sscanf(filename, "%d-", &tracknum))
				{
					printf("%02u-", tracknum);
				}
			}
			if (tmp[0]) printf("%s-", tmp);
			else		fputs("unknown-", stdout);

			strncpy(tmp, tag.songname, 30);
			strip(tmp);
			j = 0;
			for (i = 0; i < strlen(tmp); i ++)
			{
				if ('å' == tmp[i] || 'Å' == tmp[i] ||
				    'ä' == tmp[i] || 'Ä' == tmp[i])
				    tmp[j ++] = 'a';
				else if ('ö' == tmp[i] || 'Ö' == tmp[i])
				    tmp[j ++] = 'o';
				else if ('é' == tmp[i] || 'É' == tmp[i])
				    tmp[j ++] = 'e';
				else if (isalpha(tmp[i]))
					tmp[j ++] = tolower(tmp[i]);
				else if (isdigit(tmp[i]))
					tmp[j ++] = tmp[i];
				else if (' ' == tmp[i])
					tmp[j ++] = '_';
			}
			tmp[j] = 0;
			if (tmp[0]) printf("%s.mp3\"\n", tmp);
			else		fputs("unknown.mp3\"\n", stdout);
		}
		else if (isshort)
		{
			// Short output format:
			// "filename artist: title"
			strncpy(tmp, tag.artist, 30);
			strip(tmp);
			printf("%s ", filename);
			if (tmp[0]) printf("%s: ", tmp);
			
			strncpy(tmp, tag.songname, 30);
			strip(tmp);
			if (!tmp[0]) strcpy(tmp, "Unknown title");
			printf("%s\n", tmp);
		}
		else
		{
			// Long output format:
			// "filename
			//  Artist - Title
			//  [Album Year]
			//  Genre"

			// Filename
			puts(filename);

			// Artist
 			strncpy(tmp, tag.artist, 30);
			strip(tmp);
			if (!tmp[0]) strcpy(tmp, "Unknown artist");
			printf(" %s - ", tmp);

			// Song title
			strncpy(tmp, tag.songname, 30);
			strip(tmp);
			if (!tmp[0]) strcpy(tmp, "Unknown title");
			printf("%s\n", tmp);

			// Album
 			strncpy(tmp, tag.album, 30);
			strip(tmp);
			if (!tmp[0]) strcpy(tmp, "Unknown album");
			printf(" [%s", tmp);

			// Year		
			strncpy(tmp, tag.year, 4);
			tmp[4] = 0;
			strip(tmp);
			if (tmp[0])
				printf(" %s", tmp);
			fputc(']', stdout);
			fputc('\n', stdout);

			// Genre
			if (tag.genre < 80)
				printf(" %s (%u)\n", genre[tag.genre], (unsigned) tag.genre);
			else
				printf(" Unknown genre (%u)\n", (unsigned) tag.genre);

			fputc('\n', stdout);
		}
	}
}

// strip:
//  Strip excess spaces from strings
void strip(char *s)
{
	while (' ' == s[strlen(s) - 1])
		s[strlen(s) - 1] = 0;
}
