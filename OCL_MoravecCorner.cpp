// include user defining head file
#include "MoravecCorner.h"


double executionTime(cl_event &event)
{
	cl_ulong start, end;

	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);

	return (double)1.0e-6 * (end - start); 	
}


// OpenCL Vars
cl_platform_id *platforms;					
cl_platform_id cpPlatform = NULL;  			
cl_context cxGPUContext = NULL;    			
cl_device_id *cdDevice;          			
cl_command_queue cqCommandQueue = NULL;		
cl_program cpProgram = NULL;       			
cl_kernel ckKernel = NULL;         			
cl_event GPUExecution1;						
cl_event GPUExecution2;						
cl_event GPUDone2;							
cl_mem pgExpand_D    = NULL;				
cl_mem updatePixel_D = NULL;				
cl_mem IV_D          = NULL;		
cl_mem flags_D       = NULL;		     
size_t deviceListSize;					
const char* cSourceFile1 = "MoravecCorner_kernel.cl";   
const char* cSourceFile2 = "MoravecCorner_kernel.bin"; 
cl_uint numPlatforms;						
cl_int ciErr1, ciErr2;						

char input[256]  =
{ "E:\\application program\\block.bmp" };
char output[256] =
{ "E:\\application program\\Result.bmp" };


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
	printf("Hint:Convert RGB To GRAY Successfully!\n\n");
	free(data);		

	fclose(fpDestBmpfile);	

	fclose(fpSrcBmpfile);
	return 0;
}

cl_program CreateProgram(const char* fileName)
{
	#define MAX_SOURCE_SIZE (0x100000)
	char *source_str;	
	size_t source_size;  

	FILE *fp;
	fopen_s(&fp, fileName, "r");
	if (!fp)
	{  
		printf("Failed to load kernel.\n");  
		exit(1);  
	}  
	source_str = (char*)malloc(MAX_SOURCE_SIZE);  
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);  
	fclose(fp);

	cpProgram = clCreateProgramWithSource(cxGPUContext, 1, (const char **)&source_str,(const size_t *)&source_size, &ciErr1); 
	if (cpProgram == NULL)
	{
		printf("Error in clCreateProgramWithSource!!!\n\n");
		return NULL;
	}

	ciErr1 = clBuildProgram( cpProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL );
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error in clBuildProgram!!!\n\n"); 
		return NULL;
	}

	free(source_str);
	return cpProgram;
}


bool SaveProgramBinary(const char* fileName)
{
	cl_uint numDevices = 0;

	ciErr1 = clGetProgramInfo(cpProgram, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint), &numDevices, NULL);
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error querying for number of devices.");
		return false;
	}

	cl_device_id *devices = new cl_device_id[numDevices];
	ciErr1 = clGetProgramInfo(cpProgram, CL_PROGRAM_DEVICES, sizeof(cl_device_id) * numDevices,	devices, NULL);
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error querying for devices.");
		delete [] devices;
		return false;
	}

	size_t *programBinarySizes = new size_t [numDevices];
	ciErr1 = clGetProgramInfo(cpProgram, CL_PROGRAM_BINARY_SIZES, sizeof(size_t) * numDevices, programBinarySizes, NULL);
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error querying for program binary sizes.");
		delete [] devices;
		delete [] programBinarySizes;
		return false;
	}

	unsigned char **programBinaries = new unsigned char*[numDevices];
	for (cl_uint i = 0; i < numDevices; i++)
	{
		programBinaries[i] = new unsigned char[programBinarySizes[i]];
	}

	ciErr1 = clGetProgramInfo(cpProgram, CL_PROGRAM_BINARIES, sizeof(unsigned char*) * numDevices, programBinaries, NULL);
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error querying for program binaries");

		delete [] devices;
		delete [] programBinarySizes;
		for (cl_uint i = 0; i < numDevices; i++)
		{
			delete [] programBinaries[i];
		}
		delete [] programBinaries;
		return false;
	}

	for (cl_uint i = 0; i < numDevices; i++)
	{
		if (devices[i] == cdDevice[0])
		{
			FILE *fp;
			fopen_s(&fp, fileName, "wb");
			fwrite(programBinaries[i], 1, programBinarySizes[i], fp);
			fclose(fp);
			break;
		}
	}

	delete [] devices;
	delete [] programBinarySizes;
	for (cl_uint i = 0; i < numDevices; i++)
	{
		delete [] programBinaries[i];
	}
	delete [] programBinaries;
	return true;
}


cl_program CreateProgramFromBinary(const char* fileName)
{
	FILE *fp;
	fopen_s(&fp, fileName, "rb");
	if (fp == NULL)
	{
		printf("The specified kernel binary file cannot be opened!\n\n");
		return NULL;
	}

	size_t binarySize;
	cl_int binaryStatus;
	fseek(fp, 0L, SEEK_END);
	binarySize = ftell(fp);
	rewind(fp);	

	unsigned char *programBinary = new unsigned char[binarySize];
	fread(programBinary, sizeof(unsigned char), binarySize, fp);
	fclose(fp);

	cpProgram = clCreateProgramWithBinary(cxGPUContext, 1, &cdDevice[0], &binarySize, 
		                                 (const unsigned char**)&programBinary, &binaryStatus, &ciErr1);
	free(programBinary);
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error in clCreateProgramWithBinary!!!\n\n");
		return NULL;
	}

	if (binaryStatus != CL_SUCCESS)
	{
		printf("Invalid binary for device");
		return NULL;
	}

	ciErr1 = clBuildProgram( cpProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL );
	if (ciErr1 != CL_SUCCESS)
	{
		char buildLog[16384];
		printf("Error in clBuildProgram!!!\n\n");
		clGetProgramBuildInfo(cpProgram, cdDevice[0], CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);
		printf("%s\n", buildLog);
		clReleaseProgram(cpProgram);
		return NULL;
	}

	return cpProgram;
}


int main(int argc, char **argv)
{
	int i, j;			
	int baseaddr;		
	int biMODELDIM;		

LOOP:
	FILE *imageFile;						
	fopen_s( &imageFile, input, "rb" );		
	if (imageFile == NULL)
	{     
		printf("\nFile open error!\n");
		exit(0);
	}

	fseek(imageFile, sizeof( BITMAPFILEHEADER ), 0 );

	BITMAPINFOHEADER head;
	fread( &head, sizeof( BITMAPINFOHEADER ), 1, imageFile);

	RGBQUAD *pColorTable = NULL;		
	if(head.biBitCount==8)				
	{
		pColorTable = new RGBQUAD[256];		
		fread( pColorTable, sizeof( RGBQUAD ), 256, imageFile);
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

	fread(pg, sizeof(unsigned char), imageSize, imageFile);		


	unsigned char *updatePixel = (unsigned char*) malloc ( sizeof(unsigned char) * imageSize );
	if ( !updatePixel ) 
	{
		printf( "\n host memory allocation failed. \n" );
		return EXIT_FAILURE;
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

	size_t input_imageSize         = height*lineByteExpand * sizeof(float);
	size_t InterestValue_imageSize = imageSize	           * sizeof(float);		
	size_t flag_imageSize          = imageSize	           * sizeof(int);		
	size_t output_imageSize        = imageSize             * sizeof(unsigned char);


	ciErr1 = clGetPlatformIDs( 0, NULL, &numPlatforms );
	if ( ciErr1 != CL_SUCCESS )
	{
		printf("Error: Getting Platforms.(clGetPlatformsIDs)!!!\n\n");
		return EXIT_FAILURE;
	}
	else 
	{
		if( numPlatforms == 0 )
		{
			printf("No OpenCL platform found!\n\n");
			return EXIT_FAILURE;
		}
		else 
		{
			if ((platforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id))) == NULL) 
			{
				printf("Failed to allocate memory for cl_platform ID's!\n\n");
				return EXIT_FAILURE;
			}

			ciErr1 = clGetPlatformIDs (numPlatforms, platforms, NULL); 
			if ( ciErr1 != CL_SUCCESS )
			{
				printf("Error: Getting PlatformIds.(clGetPlatformsIDs)!!!\n\n");
				return EXIT_FAILURE;
			}
		
			for ( i=0; i < int(numPlatforms); ++i )
			{
				size_t size;
				clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 0	, NULL , &size);
				char *pbuff = (char*)malloc(size);
				clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, size, pbuff, NULL);
				cpPlatform = platforms[i];
				if( !strcmp( pbuff, "NVIDIA Corporation" ) || 
					!strcmp( pbuff, "AMD Corporation" )    ||
					!strcmp( pbuff, "INTEL Corporation" ))
				{
					break;
				}
				free(pbuff);
			}
			free(platforms);
		}
	}
	cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM, ( cl_context_properties ) cpPlatform, 0 };
	cl_context_properties *cprops = (NULL == cpPlatform) ? NULL : cps ;
	

	cxGPUContext = clCreateContextFromType ( cprops, CL_DEVICE_TYPE_ALL, NULL, NULL, &ciErr1 );
	if ( ciErr1 != CL_SUCCESS )
	{
		printf("Error in clCreateContext!!!\n\n");
		return EXIT_FAILURE;
	}

	ciErr1 = clGetContextInfo( cxGPUContext, CL_CONTEXT_DEVICES, 0, NULL, &deviceListSize );
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error: Getting Context Info(device list size, clGetContextInfo)\n");
		return EXIT_FAILURE;
	}
	if (deviceListSize <= 0)
	{
		printf("No devices available.");
		return NULL;
	}
	cdDevice = (cl_device_id *)malloc(deviceListSize);
	if (cdDevice == 0)
	{
		printf("Error: No devices found.\n");
		return EXIT_FAILURE;
	}
	ciErr1 = clGetContextInfo( cxGPUContext, CL_CONTEXT_DEVICES, deviceListSize, cdDevice, NULL );
	if ( ciErr1 != CL_SUCCESS )
	{
		printf("Error: Getting Context Info(device list, clGetContextInfo)\n");
		return EXIT_FAILURE;
	}

	cqCommandQueue = clCreateCommandQueue(cxGPUContext, cdDevice[0], CL_QUEUE_PROFILING_ENABLE, &ciErr1);
	if ( ciErr1 != CL_SUCCESS )
	{
		printf("Error in clCreateCommandQueue!!!\n\n");
		return EXIT_FAILURE;
	}
	printf("Attempting to create program from binary...\n");
	cpProgram = CreateProgramFromBinary(cSourceFile2);
	if (cpProgram == NULL)
	{
		printf("Binary not loaded, create from source...\n");
		cpProgram = CreateProgram(cSourceFile1);
		if (cpProgram == NULL)
		{
			printf("Failed to creat binary program");
			return 1;
		}

		printf("Save program binary for future run...\n");
		if (SaveProgramBinary(cSourceFile2) == false)
		{
			printf("Failed to write binary program");
			return 1;
		}
	}
	else
	{
		printf("Read program from binary.\n");
	}

	
	pgExpand_D    = clCreateBuffer(cxGPUContext, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, input_imageSize ,        pgExpand, &ciErr1);
	updatePixel_D = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, output_imageSize,        pg,       &ciErr2);
	ciErr1 |= ciErr2;
	IV_D          = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE,                        InterestValue_imageSize, NULL,     &ciErr2);
	ciErr1 |= ciErr2;
	flags_D       = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE,                        flag_imageSize,          NULL,     &ciErr2);
	ciErr1 |= ciErr2;
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error in clCreateBuffer!!!\n\n");
		return EXIT_FAILURE;
	}

	size_t localWorkSize1 [] = { THREAD, THREAD };
	size_t globalWorkSize1[] = { (col + THREAD - 1) / THREAD*THREAD,
								 (row + THREAD - 1) / THREAD*THREAD };		
	ckKernel = clCreateKernel( cpProgram, "CandidateCorner_kernel", &ciErr1 );
	if ( ciErr1 != CL_SUCCESS )
	{
		printf("Error in clCreateKernel_1!!!\n\n");
		return EXIT_FAILURE;
	}
	
	ciErr1  = clSetKernelArg(ckKernel, 0, sizeof(cl_mem), (void*)&pgExpand_D);
	ciErr1 |= clSetKernelArg(ckKernel, 1, sizeof(cl_mem), (void*)&IV_D);
	ciErr1 |= clSetKernelArg(ckKernel, 2, sizeof(cl_mem), (void*)&flags_D);
	ciErr1 |= clSetKernelArg(ckKernel, 3, sizeof(cl_int), (void*)&height);
	ciErr1 |= clSetKernelArg(ckKernel, 4, sizeof(cl_int), (void*)&width);
	ciErr1 |= clSetKernelArg(ckKernel, 5, sizeof(cl_int), (void*)&lineByteExpand);
	ciErr1 |= clSetKernelArg(ckKernel, 6, sizeof(cl_int), (void*)&row);
	ciErr1 |= clSetKernelArg(ckKernel, 7, sizeof(cl_int), (void*)&col);
	ciErr1 |= clSetKernelArg(ckKernel, 8, sizeof(cl_int), (void*)&lineByte);
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error in clSetKernelArg_1!!!\n\n");
		return EXIT_FAILURE;
	}
 
	ciErr1 = clEnqueueNDRangeKernel(cqCommandQueue, ckKernel, 2, 0, globalWorkSize1, localWorkSize1, 0, NULL, &GPUExecution2);
	if ( ciErr1 != CL_SUCCESS )
	{
		printf("Error in clEnqueueNDRangeKernel_1!!!\n\n");
		return EXIT_FAILURE;
	}


	size_t localWorkSize2 [] = { THREAD, THREAD };
	size_t globalWorkSize2[] = { (col + THREAD - 1) / THREAD*THREAD,
								 (row + THREAD - 1) / THREAD*THREAD };		

	ckKernel = clCreateKernel( cpProgram, "MoravecCorner_kernel", &ciErr1 );
	if ( ciErr1 != CL_SUCCESS )
	{
		printf("Error in clCreateKernel_2!!!\n\n");
		return EXIT_FAILURE;
	}
	
	ciErr1  = clSetKernelArg(ckKernel, 0, sizeof(cl_mem), (void*)&updatePixel_D);
	ciErr1 |= clSetKernelArg(ckKernel, 1, sizeof(cl_mem), (void*)&IV_D);
	ciErr1 |= clSetKernelArg(ckKernel, 2, sizeof(cl_mem), (void*)&flags_D);
	ciErr1 |= clSetKernelArg(ckKernel, 3, sizeof(cl_int), (void*)&row);
	ciErr1 |= clSetKernelArg(ckKernel, 4, sizeof(cl_int), (void*)&lineByte);
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error in clSetKernelArg_2!!!\n\n");
		return EXIT_FAILURE;
	}
 
	ciErr1 = clEnqueueNDRangeKernel(cqCommandQueue, ckKernel, 2, 0, globalWorkSize2, localWorkSize2, 0, NULL, &GPUExecution2);
	if ( ciErr1 != CL_SUCCESS )
	{
		printf("Error in clEnqueueNDRangeKernel_2!!!\n\n");
		return EXIT_FAILURE;
	}	


	ciErr1 = clEnqueueReadBuffer(cqCommandQueue, updatePixel_D, CL_TRUE, 0, output_imageSize, updatePixel, 1, &GPUExecution2, &GPUDone2);
	if (ciErr1 != CL_SUCCESS)
	{
		printf("Error in clEnqueueReadBuffer!!!\n\n");
		return EXIT_FAILURE;
	}

	fclose(imageFile);

	int colorTablesize = 0;
	if (biBitCount == 8)
	{
		colorTablesize = 1024;
	}

	lineByte = (col * biBitCount / 8 + 3) / 4 * 4;

	fopen_s(&imageFile, output, "wb");
	if(imageFile == NULL)
	{     
		printf("\nFile open error!\n");
		exit( 0 );
	}

	BITMAPFILEHEADER fileHead;
	fileHead.bfType = 0x4D42;	

	fileHead.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + colorTablesize + lineByte * row;
	fileHead.bfReserved1 = 0;
	fileHead.bfReserved2 = 0;

	fileHead.bfOffBits = 54 + colorTablesize;

	fwrite(&fileHead, sizeof(BITMAPFILEHEADER), 1, imageFile);

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

	fwrite(&head, sizeof( BITMAPINFOHEADER ), 1, imageFile);

	if (biBitCount == 8)
	{
		fwrite( pColorTable, sizeof( RGBQUAD ), 256, imageFile);
	}

	fwrite(updatePixel, lineByte * row, 1, imageFile);
	
	free(pg);
	free(updatePixel);

	clReleaseKernel(ckKernel);
	clReleaseCommandQueue(cqCommandQueue);
	clReleaseMemObject(pgExpand_D);
	clReleaseMemObject(updatePixel_D);
	clReleaseContext(cxGPUContext);
	clReleaseProgram(cpProgram);

	fclose(imageFile);

	return 1;
}