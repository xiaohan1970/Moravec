// includes, system
#include "MoravecCorner.h"

char input[256]  = { "E:\\application program\\artificial graphics1.bmp" };
char output[256] = { "E:\\application program\\Result.bmp" };


FILE *fpSrcBmpfile;		 
FILE *fpDestBmpfile;

void GetBmpHeader(PBITMAPFILEHEADER, PBITMAPINFOHEADER);	
void ChangeBmpHeader(PBITMAPFILEHEADER, PBITMAPINFOHEADER, WORD);	
void SetBmpHeader(const PBITMAPFILEHEADER, const PBITMAPINFOHEADER);	
void SetRGBQUAD();														
int  RgbToGray(char SrcBmpfile[256], char DestBmpfile[256]);			


void GetBmpHeader(PBITMAPFILEHEADER pbfheader, PBITMAPINFOHEADER pbiheader)
{
	fread(pbfheader, sizeof(BITMAPFILEHEADER), 1, fpSrcBmpfile);
	fread(pbiheader, sizeof(BITMAPINFOHEADER), 1, fpSrcBmpfile);
}

void ChangeBmpHeader(PBITMAPFILEHEADER pbfheader, PBITMAPINFOHEADER pbiheader, WORD wType)
{
	pbiheader->biBitCount = wType; 

	pbiheader->biClrUsed = (wType == 24) ? 0 : 256;
	pbfheader->bfOffBits = 54 + pbiheader->biClrUsed * sizeof(RGBQUAD);
	pbiheader->biSizeImage = ((((pbiheader->biWidth * pbiheader->biBitCount) + 31) & ~31) / 8) * pbiheader->biHeight;
	pbfheader->bfSize = pbfheader->bfOffBits + pbiheader->biSizeImage;
}

void SetBmpHeader(const PBITMAPFILEHEADER pbfheader, const PBITMAPINFOHEADER pbiheader)
{
	fwrite(pbfheader, sizeof(BITMAPFILEHEADER), 1, fpDestBmpfile);
	fwrite(pbiheader, sizeof(BITMAPINFOHEADER), 1, fpDestBmpfile);
}

void SetRGBQUAD()
{
	int i;
	RGBQUAD rgbquad[256];
	for (i = 0; i<256; i++) {
		rgbquad[i].rgbBlue = i;
		rgbquad[i].rgbGreen = i;
		rgbquad[i].rgbRed = i;
		rgbquad[i].rgbReserved = 0;
	}
	fwrite(rgbquad, 256 * sizeof(RGBQUAD), 1, fpDestBmpfile);
}

int RgbToGray(char SrcBmpfile[256], char out[])
{
	LONG w, h;
	BYTE r, g, b;
	BYTE gray;
	BYTE count24, count8;
	BYTE Bmpnul = 0;

	BITMAPFILEHEADER bmfh;	

	BITMAPINFOHEADER bmih;	

	BYTE *data;

	memset(&bmfh, 0, sizeof(BITMAPFILEHEADER));	

	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
	data = (BYTE *)malloc(3 * sizeof(BYTE));
	if (!data)
	{
		printf("Error:Can not allocate memory .\n");
		free(data);
		return -1;
	}

	printf("Input the path of SrcBmpfile:\n");

	fopen_s(&fpSrcBmpfile, input, "rb");
	if (fpSrcBmpfile == NULL)
	{
		free(data);
		return -1;
	}

	rewind(fpSrcBmpfile);
	GetBmpHeader(&bmfh, &bmih);

	printf("The contents in the file header of the BMP file:\n");
	printf("bfType:%ld\n", bmfh.bfType);
	printf("bfSize:%ld\n", bmfh.bfSize);
	printf("bfReserved1:%ld\n", bmfh.bfReserved1);
	printf("bfReserved2:%ld\n", bmfh.bfReserved2);
	printf("bfOffBits:%ld\n", bmfh.bfOffBits);
	printf("The contents in the info header:\n");
	printf("biSize:%ld\n", bmih.biSize);

	if (bmfh.bfType != 0x4D42)
	{
		printf("Error:This file is not bitmap file!\n");
		free(data);
		return -1;
	}
	if (bmih.biBitCount != 24)
	{
		printf("Error:This bmpfile is not 24bit bitmap!\n");
		free(data);
		return -1;
	}
	if (bmih.biCompression != 0)
	{
		printf("Error:This 8bit bitmap file is not BI_RGB type!\n");
		free(data);
		return -1;
	}

	printf("Input the path of the DestBmpfile:\n");	

	strcpy_s(out, 256, output);
	fopen_s(&fpDestBmpfile, output, "wb");
	if (fpDestBmpfile == NULL)
	{
		printf("Error:Open the file of DestBmpfile failed!\n");
		free(data);
		return -1;
	}
	ChangeBmpHeader(&bmfh, &bmih, 8);
	SetBmpHeader(&bmfh, &bmih);
	SetRGBQUAD();

	count24 = (4 - (bmih.biWidth * 3) % 4) % 4;
	count8 = (4 - (bmih.biWidth) % 4) % 4;

	for (h = bmih.biHeight - 1; h >= 0; h--)
	{
		for (w = 0; w<bmih.biWidth; w++)
		{
			fread(data, 3, 1, fpSrcBmpfile);
			if (feof(fpSrcBmpfile))
			{
				printf("Error:Read Pixel data failed!\n");
				free(data);
				return -1;
			}
			b = *data;
			g = *(data + 1);
			r = *(data + 2);
			gray = (299 * r + 587 * g + 114 * b) / 1000;
			if (gray>120)gray = 250;

			fwrite(&gray, sizeof(gray), 1, fpDestBmpfile);
		}
		fseek(fpSrcBmpfile, count24, SEEK_CUR);
		fwrite(&Bmpnul, 1, count8, fpDestBmpfile);
	}
	printf("Hint:Convert RGB To GRAY Successfully!\n");
	free(data);		

	fclose(fpDestBmpfile);		

	fclose(fpSrcBmpfile);
	return 0;
}


void MoravecCornerDetection
	(float *sourceExpand, unsigned char *update, int height, int width, int lineByteExpand, int row, int lineByte)
{
	int i;					
	int x, y;					
	int fx, fy;					
	int k = MODELDIM1 / 2;		
	float v[4];					
	float min, max;				
	int flag;					
	int number = 0;				

	float  *IV = (float*)malloc(sizeof(float)*row*lineByte);
	int *flags = (int*)  malloc(sizeof(int)  *row*lineByte);	
	if (!IV || !flags)
	{
		printf("\n host memory allocation failed. \n");
		exit(0);
	}

	memset(flags, 0, sizeof(int) * row*lineByte);	

	for (y = MODELDIM1 / 2; y < (height - MODELDIM1 / 2); ++y)		
	{
		for (x = MODELDIM1 / 2; x < (width - MODELDIM1 / 2); ++x)	
		{
			memset(v, 0, sizeof(float) * 4);

			//#pragma  unroll
			for (i = -k; i < k; i++)
			{
				v[0] = v[0] + (sourceExpand[ x      + (y + i)*lineByteExpand] - sourceExpand[ x          + (y + i + 1)*lineByteExpand])
					        * (sourceExpand[ x      + (y + i)*lineByteExpand] - sourceExpand[ x          + (y + i + 1)*lineByteExpand]);
				v[1] = v[1] + (sourceExpand[(x + i) + (y + i)*lineByteExpand] - sourceExpand[(x + i + 1) + (y + i + 1)*lineByteExpand])
					        * (sourceExpand[(x + i) + (y + i)*lineByteExpand] - sourceExpand[(x + i + 1) + (y + i + 1)*lineByteExpand]);
				v[2] = v[2] + (sourceExpand[(x + i) +  y     *lineByteExpand] - sourceExpand[(x + i + 1) +  y         *lineByteExpand])
					        * (sourceExpand[(x + i) +  y     *lineByteExpand] - sourceExpand[(x + i + 1) +  y         *lineByteExpand]);
				v[3] = v[3] + (sourceExpand[(x - i) + (y + i)*lineByteExpand] - sourceExpand[(x - i - 1) + (y + i + 1)*lineByteExpand])
					        * (sourceExpand[(x - i) + (y + i)*lineByteExpand] - sourceExpand[(x - i - 1) + (y + i + 1)*lineByteExpand]);
			}
			min = v[0];
			for (i = 1;i < 4;i++)
			{
				if (v[i] < min)
				{
					min = v[i];
				}
			}
			IV[(x - MODELDIM1 / 2) + (y - MODELDIM1 / 2)*lineByte] = min;

			if (min > T)
			{
				flags[(x - MODELDIM1 / 2) + (y - MODELDIM1 / 2)*lineByte] = 1;
			}
		}
	}

	for (y = MODELDIM2 / 2; y < (row - MODELDIM2 / 2); ++y)		
	{
		for (x = MODELDIM2 / 2; x < (lineByte - MODELDIM2 / 2); ++x)
		{
			if (flags[x + y*lineByte])
			{
				max = IV[x + y*lineByte];	
				flag = 1;
				//#pragma  unroll
				for (fy = (y - MODELDIM2 / 2); fy <= (y + MODELDIM2 / 2); fy++)
				{
					//#pragma  unroll
					for (fx = (x - MODELDIM2 / 2); fx <= (x + MODELDIM2 / 2); fx++)
					{
						if (flag && flags[fx + fy*lineByte] && (max < IV[fx + fy*lineByte]))
						{
							flag = 0;
						}
					}
				}
				if (flag)
				{
					update[(x - 2) + y*lineByte] = 255;  
					update[(x - 1) + y*lineByte] = 255;
					update[ x      + y*lineByte] = 255; 
					update[(x + 1) + y*lineByte] = 255; 
					update[(x + 2) + y*lineByte] = 255;

					update[x + (y - 2)*lineByte] = 255; 
					update[x + (y - 1)*lineByte] = 255;
					update[x + (y + 1)*lineByte] = 255;  
					update[x + (y + 2)*lineByte] = 255;
					number++;
				}
			}
		}
	}
	printf("number=%d", number);

	free(IV);
	free(flags);
}


/************************************************************************/
/* Program main                                                         */
/************************************************************************/
int main(int argc, char ** argv) {

	int i, j;			
	int baseaddr;		
	int biMODELDIM;		

LOOP:
	// open imageSource file
	FILE *imageFile;						
	fopen_s( &imageFile, input, "rb" );		
	if( imageFile == NULL )
	{     
		printf("\nFile open error!\n");
		exit(0);
	}


	fseek( imageFile, sizeof( BITMAPFILEHEADER ), 0 );


	BITMAPINFOHEADER head;
	fread( &head, sizeof( BITMAPINFOHEADER ), 1, imageFile );


	RGBQUAD *pColorTable = NULL;						
	
	if(head.biBitCount==8)							
		pColorTable = new RGBQUAD[256];						
		fread( pColorTable, sizeof( RGBQUAD ), 256, imageFile );
	}
	else
	{
		RgbToGray(input, input);	
		goto LOOP;					
	}


	int col = head.biWidth;
	int row = head.biHeight;
	int biBitCount = head.biBitCount;

	int lineByte = ( col * biBitCount / 8 + 3 ) / 4 * 4;

	int imageSize = lineByte * row;

	unsigned char *pg = new unsigned char[imageSize];

	// read imageSource file
	fread( pg, sizeof(unsigned char), imageSize, imageFile );


	// allocate host memory for all of pixels in whole image 
	// pointer for host memory
	unsigned char *updatePixel = (unsigned char*) malloc ( sizeof(unsigned char) * imageSize );		
	if ( !updatePixel ) 
	{
		printf( "\n host memory allocation failed. \n" );
		return EXIT_FAILURE;
	}

	for (i = 0; i < row; ++i)
	{
		for (j = 0; j < lineByte; ++j)
		{
			updatePixel[i*lineByte + j] = pg[i*lineByte + j];
		}
	}

	biMODELDIM = MODELDIM1 / 2;
	int height = row + (MODELDIM1 - 1);		
	int width  = col + (MODELDIM1 - 1);
	int lineByteExpand = (width * biBitCount / 8 + 3) / 4 * 4;
	float *pgExpand = (float*)malloc(sizeof(float)*height*lineByteExpand);	
	if (!pgExpand)
	{
		printf("\n host memory allocation failed. \n");
		return EXIT_FAILURE;
	}

	for (i = 1; i <= row; i++)		
	{
		for (j = 1; j <= col; j++)
		{
			pgExpand[((i - 1 + biMODELDIM)*lineByteExpand - 1) + biMODELDIM + j] = (float)pg[(i - 1)*lineByte + (j - 1)];
		}
	}
	for (i = 1; i <= row; i++)
	{
		for (j = 1;j <= biMODELDIM;j++)
		{
			baseaddr = (i + biMODELDIM - 1)*lineByteExpand + biMODELDIM;
			pgExpand[baseaddr - j]           = pgExpand[baseaddr + j - 1];			
			pgExpand[baseaddr + col - 1 + j] = pgExpand[baseaddr + col - j];			
		}
	}
	for (i = 1; i <= biMODELDIM; i++)
	{
		for (j = 0;j < width;j++)
		{
			baseaddr = (biMODELDIM + i - 1)*lineByteExpand;
			pgExpand[(biMODELDIM - i)*lineByteExpand + j] = pgExpand[baseaddr + j];		
	
			pgExpand[(biMODELDIM + row + i - 1)*lineByteExpand + j] =
			pgExpand[(biMODELDIM + row - i)*lineByteExpand + j];					
		}
	}


	clock_t start, finish;
	double duration;
	start = clock();

	for ( int loop = 0 ; loop < LOOP_ADD_TIME;  loop++ ) { 

	MoravecCornerDetection(pgExpand, updatePixel, height, width, lineByteExpand, row, lineByte);

	}

	finish = clock();
	duration = (double)(finish - start) * 1000 / CLOCKS_PER_SEC;
	printf("\nMoravecCornerDetection CPU time is %f ms\n", duration / LOOP_ADD_TIME);


	// close imageSource file
	fclose( imageFile );


	int colorTablesize = 0;
	if ( biBitCount == 8)
	{
		colorTablesize = 1024;
	}


	lineByte = ( col * biBitCount / 8 + 3 ) / 4 * 4;


	// open imageGoal file
	fopen_s( &imageFile, output, "wb" );
	if( imageFile == NULL )
	{     
		printf("\nFile open error!\n");
		exit( 0 );
	}


	BITMAPFILEHEADER fileHead;
	fileHead.bfType = 0x4D42;	//bmpÀàÐÍ

	fileHead.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + colorTablesize + lineByte * row;
	fileHead.bfReserved1 = 0;
	fileHead.bfReserved2 = 0;

	fileHead.bfOffBits = 54 + colorTablesize;

	fwrite( &fileHead, sizeof( BITMAPFILEHEADER ), 1, imageFile );

	//BITMAPINFOHEADER head;
	head.biBitCount = biBitCount;
	head.biClrImportant = 0;
	head.biClrUsed = 0;
	head.biCompression = 0;
	head.biHeight = row;
	head.biPlanes = 1;
	head.biSize = 40;
	head.biSizeImage = lineByte * row;
	head.biWidth = col;
	head.biXPelsPerMeter = 0;
	head.biYPelsPerMeter = 0;

	fwrite( &head, sizeof( BITMAPINFOHEADER ), 1, imageFile );

	if (biBitCount == 8)
	{
		fwrite( pColorTable, sizeof( RGBQUAD ), 256, imageFile );
	}

	fwrite(updatePixel, lineByte * row, 1, imageFile);


	// free host memory
	free( pg );
	free( updatePixel );
	free( pgExpand );

	// close file
	fclose( imageFile );

	return 0;
}