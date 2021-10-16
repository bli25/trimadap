#ifndef _IZLIB_H
#define _IZLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "igzip_lib.h"

#ifndef GZ_BUF_SIZE
#define GZ_BUF_SIZE (1<<22)
#endif

#ifndef GZ_HDR_SIZE
#define GZ_HDR_SIZE (1<<16)
#endif

typedef struct
{
	FILE *fp;
	int is_plain;
	struct isal_gzip_header *gz_hdr;
	struct inflate_state *state;
	uint8_t *inbuf;
	size_t inbuf_size;
} gzFile_t;

typedef gzFile_t* gzFile;

int is_gz(FILE* fp);
gzFile gzopen(const char *in, const char *mode);
gzFile gzdopen(int fd, const char *mode);
void gzclose(gzFile fp);
int gzread(gzFile fp, void* buf, size_t len);

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

gzFile gzopen(const char *in, const char *mode)
{
	gzFile fp = calloc(1, sizeof(gzFile_t));
	if(!(fp->fp = fopen(in, mode)))
	{
		gzclose(fp);
		return NULL;
	}
	if ((fp->is_plain = !is_gz(fp->fp))) return fp;
	fp->gz_hdr = calloc(1, sizeof(struct isal_gzip_header));
	fp->state = calloc(1, sizeof(struct inflate_state));
	fp->inbuf_size = GZ_BUF_SIZE;
	fp->inbuf = malloc(fp->inbuf_size * sizeof(uint8_t));
	isal_gzip_header_init(fp->gz_hdr);
	isal_inflate_init(fp->state);
	fp->state->crc_flag = ISAL_GZIP_NO_HDR_VER;
	fp->state->next_in = fp->inbuf;
	fp->state->avail_in = fread(fp->state->next_in, 1, fp->inbuf_size, fp->fp);
	int ret = isal_read_gzip_header(fp->state, fp->gz_hdr);
	if (ret != ISAL_DECOMP_OK)
	{
		gzclose(fp);
		fp = NULL;
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
	if (!fp) return;
	if (fp->gz_hdr) free(fp->gz_hdr);
	if (fp->state) free(fp->state);
	if (fp->inbuf) free(fp->inbuf);
	if (fp->fp) fclose(fp->fp);
	free(fp);
}

int gzread(gzFile fp, void *buf, size_t len)
{
	int buf_data_len = 0, ret;
	if (fp->is_plain && !feof(fp->fp))
		return fread((uint8_t *)buf, 1, len, fp->fp);
	while (buf_data_len == 0)
	{
		if (feof(fp->fp) && fp->state->avail_in == 0) return buf_data_len;
		if (fp->state->avail_in == 0)
		{
			fp->state->next_in = fp->inbuf;
			fp->state->avail_in = fread(fp->state->next_in, 1, fp->inbuf_size, fp->fp);
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
				fp->state->next_in = fp->inbuf;
				fp->state->avail_in = fread(fp->state->next_in, 1, fp->inbuf_size, fp->fp);
			}
			else if (fp->state->avail_in >= GZ_HDR_SIZE)
			{
				uint8_t* old_next_in = fp->state->next_in;
				size_t old_avail_in = fp->state->avail_in;
				isal_inflate_reset(fp->state);
				fp->state->avail_in = old_avail_in;
				fp->state->next_in = old_next_in;
			}
			else
			{
				size_t old_avail_in = fp->state->avail_in;
				memmove(fp->inbuf, fp->state->next_in, fp->state->avail_in);
				size_t added = 0;
				if (!feof(fp->fp))
					added = fread(fp->inbuf + fp->state->avail_in, 1, fp->inbuf_size - fp->state->avail_in, fp->fp);
				isal_inflate_reset(fp->state);
				fp->state->next_in = fp->inbuf;
				fp->state->avail_in = old_avail_in + added;
			}
			if ((ret = isal_read_gzip_header(fp->state, fp->gz_hdr)) != ISAL_DECOMP_OK)
				return -3;
		}
	}
	return buf_data_len;
}

#endif
