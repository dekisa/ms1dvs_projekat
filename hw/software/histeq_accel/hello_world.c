#include <stdio.h>
#include <stdlib.h>
#include <altera_avalon_performance_counter.h>
#include <system.h>

int main()
{
	FILE *fp;

	unsigned char *input_image, *output_image;
	int i, j;

	unsigned int width;
	unsigned int height;

	unsigned int tl_x, tl_y, br_x, br_y, roi_width, roi_height;

	/*Coordinates of top-left and bottom-right corner of the target block*/
	tl_x = 32; br_x = 96;
	tl_y = 32; br_y = 96;

	/*Width and height of the target block*/
	roi_height = br_x - tl_x + 1;
	roi_width = br_y - tl_y + 1;

	PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
	PERF_START_MEASURING(PERFORMANCE_COUNTER_0_BASE);

	printf("Hello from Nios II!\n");

	fp = fopen("/mnt/host/orig64.bin", "rb");

	fread(&width, sizeof(width), 1, fp);
	fread(&height, sizeof(height), 1, fp);
	printf("Image width: %d heigth: %d\n", width, height);

	input_image = (unsigned char*) malloc(width*height);
	output_image = (unsigned char*) malloc(width*height);

	fread(input_image, 1, width*height, fp);
	fclose(fp);

    printf("\nReading input done\n");

    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE,1);

	/*Processing loop*/
	for (i = 0; i< height; i++)
	{
		for (j = 0; j< width; j++)
		{
			/*Invert pixel if it is in the target block, else just copy unchanged value*/
			if (i>= tl_x && i < br_x && j >= tl_y && j < br_y )
				output_image[i*width + j] = 255 - input_image[i*width + j];
			else
				output_image[i*width + j] = input_image[i*width + j];

		}
	}
	PERF_END(PERFORMANCE_COUNTER_0_BASE,1);
	perf_print_formatted_report(PERFORMANCE_COUNTER_0_BASE,alt_get_cpu_freq(),1, "processing");
	printf("\nProcessing done\n");

	/*Open output file and write result from output buffer to the output file*/
	fp = fopen("/mnt/host/orig64_neg.bin", "wb");
	fwrite(&width, sizeof(width), 1, fp);
	fwrite(&height, sizeof(height), 1, fp);
	fwrite(output_image, 1, width*height, fp);
	fclose(fp);

	printf("\nWriting output done\n");

	/*Clean memory*/
	free(input_image);
	free(output_image);

	printf("\nFinished\n");

	return 0;
}
