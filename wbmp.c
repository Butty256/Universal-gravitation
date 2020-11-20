#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wbmp.h"

int writeBmp(const Image *img, const char *filename)
{
	FILE* fp;
	unsigned char hbuf[HEADERSIZE];
	unsigned char *bmpl;
	int i, j;
	int rwidth = img->width * 3 + img->width % 4;
	unsigned long dsize = img->height * rwidth;
	unsigned long fsize = dsize + HEADERSIZE;
	unsigned long osize = HEADERSIZE;
	unsigned long isize = INFOHEADERSIZE;
	unsigned short pn = 1;
	unsigned short cbit = 24;
	unsigned long cmp = 0;
	unsigned long xppm = 1;
	unsigned long yppm = 1;

	if ((fp = fopen(filename, "wb")) == NULL) return 1;

	/* Header */
	hbuf[0] = 'B', hbuf[1] = 'M';
	memcpy(hbuf + 2, &fsize, sizeof(unsigned long));
	hbuf[6] = hbuf[7] = hbuf[8] = hbuf[9] = 0;
	memcpy(hbuf + 10, &osize, sizeof(unsigned long));
	memcpy(hbuf + 14, &isize, sizeof(unsigned long));
	hbuf[15] = hbuf[16] = hbuf[17] = 0;
	memcpy(hbuf + 18, &img->width, sizeof(unsigned long));
	memcpy(hbuf + 22, &img->height, sizeof(unsigned long));
	memcpy(hbuf + 26, &pn, sizeof(unsigned short));
	memcpy(hbuf + 28, &cbit, sizeof(unsigned short));
	memcpy(hbuf + 30, &cmp, sizeof(unsigned long));
	memcpy(hbuf + 34, &dsize, sizeof(unsigned long));
	memcpy(hbuf + 38, &xppm, sizeof(unsigned long));
	memcpy(hbuf + 42, &yppm, sizeof(unsigned long));
	hbuf[46] = hbuf[47] = hbuf[48] = hbuf[49] = hbuf[50] = hbuf[51] = hbuf[52] = hbuf[53] = 0;
	fwrite(hbuf, sizeof(unsigned char), HEADERSIZE, fp);

	/* RGB data */
	if ((bmpl = (unsigned char *)malloc(sizeof(unsigned char) * rwidth)) == NULL) {fclose(fp); return 1;}
	for (i = 0; i < img->height; i++)
	{
		for (j = 0; j < img->width; j++)
		{
			bmpl[j * 3] = img->data[(img->height - i - 1) * img->width + j].b;
			bmpl[j * 3 + 1] =img->data[(img->height - i - 1) * img->width + j].g;
			bmpl[j * 3 + 2] = img->data[(img->height - i - 1) * img->width + j].r;
		}
		for(j = img->width * 3; j < rwidth; j++) bmpl[j] = 0;
		fwrite(bmpl, sizeof(unsigned char), rwidth, fp);
	}
	free(bmpl);

	fclose(fp);

	return 0;
}

Image *createImage(unsigned long width, unsigned long height)
{
	Image *img;

	if ((img = (Image *)malloc(sizeof(Image))) == NULL) return NULL;
	if ((img->data = (RGB*)malloc(sizeof(RGB) * width * height)) == NULL) {free(img); return NULL;}

	img->width = width;
	img->height = height;

	return img;
}

void freeImage(Image *img)
{
	free(img->data);
	free(img);
}
