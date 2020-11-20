#ifndef _WBMP_H_
#define _WBMP_H_

#define FILEHEADERSIZE 14
#define INFOHEADERSIZE 40
#define HEADERSIZE (FILEHEADERSIZE + INFOHEADERSIZE)

struct RGB
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};
typedef struct RGB RGB;

struct Image
{
	unsigned long width;
	unsigned long height;
	RGB *data;
};
typedef struct Image Image;

int writeBmp(const Image *img, const char *filename);
Image *createImage(unsigned long width, unsigned long height);
void freeImage(Image *img);

#endif /* _WBMP_H_ */
