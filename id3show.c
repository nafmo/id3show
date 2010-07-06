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
#include <stdint.h>
#include <stdlib.h>

#define FALSE	0
#define TRUE	1

#pragma pack(1)

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

typedef struct
{
	char			ident[3];
	unsigned char	major,
					minor,
					flags;
	uint32_t		safe_len;
} id3v2_header_t, id3v2_footer_t;

typedef struct
{
	char			frame_id[4];
	uint32_t		safe_len;
	uint16_t		flags[2];
} id3v2_3_frame_t;

typedef struct
{
	char			frame_id[3];
	uint32_t		len: 24;
} id3v2_2_frame_t;

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
int showid3v2(FILE *, const char *, int, int);
void strip(char *);
uint32_t decode_safe(uint32_t);
void id3v2_get_text_data(char *, size_t, uint32_t, FILE *);

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

	if (!showid3v2(fp, filename, isshort, ismv))
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

// showid3v2:
//  shows an ID3v2 tag
int showid3v2(FILE *fp, const char *filename, int isshort, int ismv)
{
	id3v2_header_t	header, footer;
	uint32_t		size;
	int				use_unsynchronisation;
	char			title[64], artist[64], track[16];

//printf("# v2: locating header\n");
	// Try finding header at the top of the file
	if (-1 == fseek(fp, 0, SEEK_SET))
		return 0;
	if (0 == fread(&header, sizeof(header), 1, fp))
		return 0;

	if ('I' != header.ident[0] || 'D' != header.ident[1] ||
		'3' != header.ident[2] || header.major > 4 ||
		header.minor == 0xFF)
	{
//printf("# v2: locating footer 1\n");
		// Try finding a footer at the end of the file
		if (-1 == fseek(fp, -sizeof(id3v2_footer_t), SEEK_END))
			return 0;
		if (0 == fread(&footer, sizeof(footer), 1, fp))
			return 0;
		if ('3' != footer.ident[0] || 'D' != footer.ident[1] ||
			'I' != footer.ident[2] || footer.major > 4 ||
			footer.minor == 0xFF)
		{
//printf("# v2: locating footer 2\n");
			// Try finding a footer before a ID3v1 tag
			if (-1 == fseek(fp, -(sizeof(id3v2_footer_t) + sizeof(id3_t)), SEEK_END))
				return 0;
			if (0 == fread(&footer, sizeof(footer), 1, fp))
				return 0;
			if ('3' != footer.ident[0] || 'D' != footer.ident[1] ||
				'I' != footer.ident[2] || footer.major > 4 ||
				footer.minor == 0xFF)
				return 0;
		}

//printf("# v2: locating header 2\n");
		// Seek back and read the header
		if (-1 == fseek(fp, -(decode_safe(footer.safe_len) + 20), SEEK_CUR))
			return 0;
		if (0 == fread(&header, sizeof(header), 1, fp))
			return 0;
		if ('I' != header.ident[0] || 'D' != header.ident[1] ||
			'3' != header.ident[2] || header.major != footer.major ||
			header.minor != footer.minor ||
			header.safe_len != footer.safe_len)
			return 0;
	}

	// Only support 2.2, 2.3 and 2.4
	if (header.major < 2)
		return 0;

	// Valid header found, now start reading
#define ID3V2_USE_UNSYNCH		0x80
#define ID3V2_EXTENDED_HEADER	0x40
	if (header.flags & ID3V2_USE_UNSYNCH)
	{
		printf("# %s: Unsynchronise not supported (yet)\n", filename);
		return 0;
	}

	size = decode_safe(header.safe_len);
//printf("# v2.%u.%u: tag len = %u; unsynch = %u\n", header.major, header.minor, size, use_unsynchronisation);
	if (header.major >= 3)
	{
		if (header.flags & ID3V2_EXTENDED_HEADER)
		{
			uint32_t ext_header_len;

			// Discard extended header
			if (0 == fread(&ext_header_len, sizeof(ext_header_len), 1, fp))
				return 0;
			if (-1 == fseek(fp, decode_safe(ext_header_len) - 4, SEEK_CUR))
				return 0;
			size -= decode_safe(ext_header_len);
//printf("# v2.3: ignoring extheader len = %u\n", decode_safe(ext_header_len));
		}

		// Read 2.3+ frames
		while (size > 0)
		{
			id3v2_3_frame_t	frame_header;
			uint32_t		frame_size;
//printf("# v2.3: reading frame; left to read = %u\n", size);
			if (0 == fread(&frame_header, sizeof(frame_header), 1, fp))
				return 0;
			frame_size = decode_safe(frame_header.safe_len);
			// Decode known frames
			if (!frame_header.frame_id[0] && !frame_header.frame_id[1] &&
			    !frame_header.frame_id[2] && !frame_header.frame_id[3])
			{
				// Stop on empty frame
				break;
			}
			else if (strncmp(frame_header.frame_id, "TIT2", 4) == 0)
			{
				// Title
				id3v2_get_text_data(title, sizeof (title), frame_size, fp);
			}
			else if (strncmp(frame_header.frame_id, "TPE1", 4) == 0)
			{
				// Artist
				id3v2_get_text_data(artist, sizeof (artist), frame_size, fp);
			}
			else if (strncmp(frame_header.frame_id, "TRCK", 4) == 0)
			{
				// Track
				id3v2_get_text_data(track, sizeof (track), frame_size, fp);
			}
			else
			{
				// Skip unknown frame
//printf("# v2: skipping '%.4s' len = %u\n", frame_header.frame_id, frame_size);
				if (-1 == fseek(fp, frame_size - sizeof(frame_header), SEEK_CUR))
					return 0;
			}
			size -= frame_size;
		}
	}
	else
	{
		// Read 2.2 frames
		while (size > 0)
		{
			id3v2_2_frame_t	frame_header;
			uint32_t		frame_size;

//printf("# v2.2: reading frame; left to read = %u\n", size);
			if (0 == fread(&frame_header, sizeof(frame_header), 1, fp))
				return 0;
			frame_size = ((frame_header.len & 0xFF0000) >> 16) |
			             ((frame_header.len & 0xFF00)) |
			             ((frame_header.len & 0xFF) << 16);
			// Decode known frames
			if (!frame_header.frame_id[0] && !frame_header.frame_id[1] &&
			    !frame_header.frame_id[2])
			{
				// Stop on empty frame
				break;
			}
			else if (strncmp(frame_header.frame_id, "TT2", 3) == 0)
			{
				// Title
				id3v2_get_text_data(title, sizeof (title), frame_size, fp);
			}
			else if (strncmp(frame_header.frame_id, "TP1", 3) == 0)
			{
				// Artist
				id3v2_get_text_data(artist, sizeof (artist), frame_size, fp);
			}
			else if (strncmp(frame_header.frame_id, "TRK", 3) == 0)
			{
				// Track
				id3v2_get_text_data(track, sizeof (track), frame_size, fp);
			}
			else
			{
				// Skip unknown frame
				if (!frame_header.frame_id[0] && !frame_header.frame_id[1] &&
				    !frame_header.frame_id[2])
					break;
//printf("# v2.2: skipping '%.3s' len = %u\n", frame_header.frame_id, frame_size);
				if (-1 == fseek(fp, frame_size, SEEK_CUR))
					return 0;
				size -= frame_size;
			}
			
		}
	}

	if (artist[0] && title[0])
	{
		if (ismv)
		{
			int i, j;
			// mv output format:
			// "mv filename num-artist-title.mp3"
			j = 0;
			for (i = 0; i < strlen(artist); i ++)
			{
				if ('å' == artist[i] || 'Å' == artist[i] ||
				    'ä' == artist[i] || 'Ä' == artist[i])
				    artist[j ++] = 'a';
				else if ('ö' == artist[i] || 'Ö' == artist[i])
				    artist[j ++] = 'o';
				else if ('é' == artist[i] || 'É' == artist[i])
				    artist[j ++] = 'e';
				else if (isalpha(artist[i]))
					artist[j ++] = tolower(artist[i]);
				else if (isdigit(artist[i]))
					artist[j ++] = artist[i];
				else if (' ' == artist[i])
					artist[j ++] = '_';
			}
			artist[j] = 0;
			printf("mv -i \"%s\" \"", filename);
			if (track[0])
			{
				char *p;
				if (p = strrchr(track, '/'))
				{
					*p = 0;
					if (p - track < 2)
						putchar('0');
				}
				printf("%s-", track);
			}
			printf("%s-", artist);

			j = 0;
			for (i = 0; i < strlen(title); i ++)
			{
				if ('å' == title[i] || 'Å' == title[i] ||
				    'ä' == title[i] || 'Ä' == title[i])
				    title[j ++] = 'a';
				else if ('ö' == title[i] || 'Ö' == title[i])
				    title[j ++] = 'o';
				else if ('é' == title[i] || 'É' == title[i])
				    title[j ++] = 'e';
				else if (isalpha(title[i]))
					title[j ++] = tolower(title[i]);
				else if (isdigit(title[i]))
					title[j ++] = title[i];
				else if (' ' == title[i])
					title[j ++] = '_';
			}
			title[j] = 0;
			printf("%s.mp3\"\n", title);
		}
		else /*if (isshort)*/
		{
			// Short output format:
			// "filename artist: title"
			printf("%s %s: %s\n", filename, artist, title);
		}
/*
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
*/
	}


	/*
typedef struct
{
	char			frame_id[4];
	uint32_t		safe_len;
	uint16_t		flags[2];
} id3v2_3_frame_t;

typedef struct
{
	char			frame_id[3];
	uint32_t		len: 24;
} id3v2_2_frame_t;
	 */

	// All went well
	return 1;
}

// strip:
//  Strip excess spaces from strings
void strip(char *s)
{
	while (' ' == s[strlen(s) - 1])
		s[strlen(s) - 1] = 0;
}

// decode_safe:
//  Decode ID3v2 synchsafe integer
uint32_t decode_safe(uint32_t safeint)
{
	// Synchsafe integers have seven bits of data per byte
	uint32_t decoded =
		((safeint & 0x7F) << 21) |
		((safeint & 0x7F00) << 6) |
		((safeint & 0x7F0000) >> 9) |
		((safeint & 0x7F000000) >> 24);
	// FIXME: Not endian safe :-)
//printf("# decode_safe(%04x) => %04x\n", safeint, decoded);
	return decoded;
}

// id3v2_get_text_data:
//  Decode ID3v2 text string
void id3v2_get_text_data(char *dest, size_t dest_len, uint32_t len, FILE *fp)
{
	char *buf;

	// Clear string
	memset(dest, 0, dest_len);

	buf = (char *) malloc(len);
	if (!buf)
	{
		// Failed to allocate, just skip the data
		fseek(fp, dest_len, SEEK_CUR);
		return;
	}

	// Get text data
	if (fread(buf, len, 1, fp))
	{
		// FIXME: Read ISO 8859-1 and UTF-8 straight through
		if (buf[0] == 0x00 || buf[0] == 0x03)
			strncpy(dest, buf + 1, dest_len - 1);
		else if (buf[0] == 0x01 || buf[0] == 0x02)
		{
			// FIXME: Copy UTF-16 stream by chopping to eight-bit
			int i = 0;

			if (buf[0] == 0x02)
				i = 1;
			else if (buf[1] == 0xFF)
				i = 3;
			else
				i = 2;
			while (i < len && dest_len > 1)
			{
				*dest = buf[i];
				i += 2;
				-- dest_len;
			}
		}
	}
//printf("# id3v2: got string '%s'\n", dest);

	free(buf);
}
