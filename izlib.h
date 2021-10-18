#ifndef iZLIB_H
#define iZLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "igzip_lib.h"

#ifndef UNIX
#define UNIX 3
#endif

#ifndef BUF_SIZE
#define BUF_SIZE (1<<22)
#endif

#ifndef HDR_SIZE
#define HDR_SIZE (1<<16)
#endif

#ifndef MIN_COM_LVL
#define MIN_COM_LVL 0
#endif

#ifndef MAX_COM_LVL
#define MAX_COM_LVL 3
#endif

#ifndef COM_LVL_DEFAULT
#define COM_LVL_DEFAULT 3 // was 2
#endif

const int com_lvls[4] = {
	ISAL_DEF_LVL0_DEFAULT,
	ISAL_DEF_LVL1_DEFAULT,
	ISAL_DEF_LVL2_DEFAULT,
	ISAL_DEF_LVL3_DEFAULT
};

typedef struct
{
	FILE *fp;
	char *mode;
	int is_plain;
	struct isal_gzip_header *gzip_header;
	struct inflate_state *state;
	struct isal_zstream *zstream;
	uint8_t *bufi;
	size_t bufi_size;
	uint8_t *bufo;
	size_t bufo_size;
} gzFile_t;

typedef gzFile_t* gzFile;

int is_gz(FILE* fp);
uint32_t get_posix_filetime(FILE* fp);
gzFile gzopen(const char *in, const char *mode);
gzFile gzdopen(int fd, const char *mode);
int gzread(gzFile fp, void *buf, size_t len);
int gzwrite(gzFile fp, void *buf, size_t len);
int set_compress_level(gzFile fp, int level);
void gzclose(gzFile fp);

int is_gz(FILE* fp)
{
	if(!fp) return 0;
	char buf[2];
	int gzip = 0;
	if(fread(buf, 1, 2, fp) == 2){
		if(((int)buf[0] == 0x1f) && ((int)(buf[1]&0xFF) == 0x8b)) gzip = 1;
	}
	fseek(fp, 12, SEEK_SET);
	if(fread(buf, 1, 2, fp) == 2){
		if((int)buf[0] == 0x42 && (int)(buf[1]&0xFF) == 0x43) gzip = 2;
	}
	fseek(fp, 0, SEEK_SET);
	return gzip;
}

uint32_t get_posix_filetime(FILE* fp)
{
	struct stat file_stats;
	fstat(fileno(fp), &file_stats);
	return file_stats.st_mtime;
}

gzFile gzopen(const char *in, const char *mode)
{
	gzFile fp = calloc(1, sizeof(gzFile_t));
	fp->fp = fopen(in, mode);
	if(!fp->fp)
	{
		gzclose(fp);
		return NULL;
	}
	fp->mode = strdup(mode);
	// plain file
	if(*mode == 'r')
	{
		fp->is_plain = !is_gz(fp->fp);
		if (fp->is_plain) return fp;
	}
	// gz file
	fp->gzip_header = calloc(1, sizeof(struct isal_gzip_header));
	isal_gzip_header_init(fp->gzip_header);
	if (*mode == 'r') // read
	{
		fp->state = calloc(1, sizeof(struct inflate_state));
		fp->bufi_size = BUF_SIZE;
		fp->bufi = malloc(fp->bufi_size * sizeof(uint8_t));
		isal_inflate_init(fp->state);
		fp->state->crc_flag = ISAL_GZIP_NO_HDR_VER;
		fp->state->next_in = fp->bufi;
		fp->state->avail_in = fread(fp->state->next_in, 1, fp->bufi_size, fp->fp);
		int ret = isal_read_gzip_header(fp->state, fp->gzip_header);
		if(ret != ISAL_DECOMP_OK)
		{
			gzclose(fp);
			return NULL;
		}
	}
	else if (*mode == 'w') // write
	{
		fp->gzip_header->os = UNIX; // FIXME auto parse OS
		fp->gzip_header->time = get_posix_filetime(fp->fp);
		fp->gzip_header->name = strdup(in); 
		fp->gzip_header->name_buf_len = strlen(fp->gzip_header->name) + 1;
		fp->bufo_size = BUF_SIZE;
		fp->bufo = calloc(fp->bufo_size, sizeof(uint8_t));
		fp->zstream = calloc(1, sizeof(struct isal_zstream));
		isal_deflate_init(fp->zstream);
		fp->zstream->avail_in = 0;
		fp->zstream->flush = NO_FLUSH;
		fp->zstream->level = COM_LVL_DEFAULT;
		fp->zstream->level_buf_size = com_lvls[fp->zstream->level];
		fp->zstream->level_buf = calloc(fp->zstream->level_buf_size, sizeof(uint8_t));
		fp->zstream->gzip_flag = IGZIP_GZIP_NO_HDR;
		fp->zstream->avail_out = fp->bufo_size;
		fp->zstream->next_out = fp->bufo;
		int ret = isal_write_gzip_header(fp->zstream, fp->gzip_header);
		if(ret != ISAL_DECOMP_OK)
		{
			gzclose(fp);
			return NULL;
		}
	}
	return fp;
}

gzFile gzdopen(int fd, const char *mode)
{
	char *path;         /* identifier for error messages */
	gzFile gz;

	if (fd == -1 || (path = (char *)malloc(7 + 3 * sizeof(int))) == NULL)
		return NULL;
#if !defined(NO_snprintf) && !defined(NO_vsnprintf)
	(void)snprintf(path, 7 + 3 * sizeof(int), "<fd:%d>", fd);
#else
	sprintf(path, "<fd:%d>", fd);   /* for debugging */
#endif
	gz = gzopen(path, mode);
	free(path);
	return gz;
}

void gzclose(gzFile fp)
{
	if(!fp) return;
	if(fp->mode) free(fp->mode);
	if(fp->zstream && fp->fp) gzwrite(fp, NULL, 0);
	if(fp->gzip_header)
	{
		if(fp->gzip_header->name) free(fp->gzip_header->name);
		free(fp->gzip_header);
	}
	if(fp->state) free(fp->state);
	if(fp->bufi) free(fp->bufi);
	if(fp->bufo) free(fp->bufo);
	if(fp->zstream){
		if(fp->zstream->level_buf) free(fp->zstream->level_buf);
		free(fp->zstream);
	}
	if(fp->fp) fclose(fp->fp);
	free(fp);
}

int gzread(gzFile fp, void *buf, size_t len)
{
	int buf_data_len = 0, ret;
	if (fp->is_plain)
	{
		if(!feof(fp->fp)) buf_data_len = fread((uint8_t *)buf, 1, len, fp->fp);
		return buf_data_len;
	}
	while (buf_data_len == 0)
	{
		if (feof(fp->fp) && !fp->state->avail_in) return buf_data_len;
		if (!fp->state->avail_in)
		{
			fp->state->next_in = fp->bufi;
			fp->state->avail_in = fread(fp->state->next_in, 1, fp->bufi_size, fp->fp);
		}
		fp->state->next_out = (uint8_t *)buf;
		fp->state->avail_out = len;
		int ret = isal_inflate(fp->state);
		if (ret != ISAL_DECOMP_OK) return -3;
		buf_data_len = fp->state->next_out - (uint8_t *)buf;
		if (feof(fp->fp) || fp->state->avail_in > 0) break;
	}
	if (fp->state->block_state == ISAL_BLOCK_FINISH)
	{
		if (!feof(fp->fp) || fp->state->avail_in > 0)
		{
			if (fp->state->avail_in == 0)
			{
				isal_inflate_reset(fp->state);
				fp->state->next_in = fp->bufi;
				fp->state->avail_in = fread(fp->state->next_in, 1, fp->bufi_size,
						fp->fp);
			}
			else if (fp->state->avail_in >= HDR_SIZE)
			{
				uint8_t *old_next_in = fp->state->next_in;
				size_t old_avail_in = fp->state->avail_in;
				isal_inflate_reset(fp->state);
				fp->state->avail_in = old_avail_in;
				fp->state->next_in = old_next_in;
			}
			else
			{
				size_t old_avail_in = fp->state->avail_in;
				memmove(fp->bufi, fp->state->next_in, fp->state->avail_in);
				size_t added = 0;
				if (!feof(fp->fp))
					added = fread(fp->bufi + fp->state->avail_in, 1,
							fp->bufi_size - fp->state->avail_in, fp->fp);
				isal_inflate_reset(fp->state);
				fp->state->next_in = fp->bufi;
				fp->state->avail_in = old_avail_in + added;
			}
			if ((ret = isal_read_gzip_header(fp->state, fp->gzip_header)) != ISAL_DECOMP_OK)
				return -3;
		}
	}
	return buf_data_len;
}

int set_compress_level(gzFile fp, int level)
{
	if (!fp || !fp->mode || *fp->mode != 'w') return -1;
	if (level < MIN_COM_LVL || level > MAX_COM_LVL) return -1;
	if (fp->zstream->level != level)
	{
		fp->zstream->level = level;
		fp->zstream->level_buf_size = com_lvls[fp->zstream->level];
		fp->zstream->level_buf = realloc(fp->zstream->level_buf,
				fp->zstream->level_buf_size * sizeof(uint8_t));
	}
	return 0;
}

int gzwrite(gzFile fp, void *buf, size_t _len)
{
	fp->zstream->next_in = (uint8_t *)buf;
	fp->zstream->avail_in = _len;
	fp->zstream->end_of_stream = !buf;
	size_t len = 0;
	do
	{
		if(!fp->zstream->next_out)
		{
			fp->zstream->next_out = fp->bufo;
			fp->zstream->avail_out = fp->bufo_size;
		}
		int ret = isal_deflate(fp->zstream);
		if (ret != ISAL_DECOMP_OK) return -3;
		len += fwrite(fp->bufo, 1, fp->zstream->next_out - fp->bufo, fp->fp);
		fp->zstream->next_out = NULL;
	} while (!fp->zstream->avail_out);
	return len;
}

#endif
