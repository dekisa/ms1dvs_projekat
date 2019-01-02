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

	unsigned int tl_x, tl_y, br_x, br_y, roi_width, roi_height, roi_area;

	unsigned int hist[256] = {0};
	unsigned int cummhist[256] = {0};

	/*Coordinates of top-left and bottom-right corner of the target block*/
	tl_x = 0; br_x = 63;
	tl_y = 0; br_y = 63;

	/*Width and height of the target block*/
	roi_height = br_x - tl_x + 1;
	roi_width = br_y - tl_y + 1;
	roi_area = roi_height * roi_width;

	PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
	PERF_START_MEASURING(PERFORMANCE_COUNTER_0_BASE);

	printf("Hello from Nios II!\n");

	printf ("Region of interest width: %d roi_height: %d, area: %d\n", roi_width, roi_height, roi_area);

	fp = fopen("/mnt/host/low_contrast64.bin", "rb");

	fread(&width, sizeof(width), 1, fp);
	fread(&height, sizeof(height), 1, fp);
	printf("Image width: %d heigth: %d\n", width, height);

	//TODO: Proveriti tacke regiona
//	if(tl_x < 0 || tl_y < 0)
//		printf("Invalid top left coordinate for region of interest\n")
//	if(br_x < 0 || br_y < 0)
//			printf("Invalid top left coordinate for region of interest\n")

	input_image = (unsigned char*) malloc(width*height);
	output_image = (unsigned char*) malloc(width*height);

	fread(input_image, 1, width*height, fp);
	fclose(fp);

    printf("\nReading input done\n");

	/* Histogram */

	//Measure time it takes for histogram to be calculated
    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE,1);

	/*Histogram calculation loop*/
	for (i = 0; i< height; i++)
	{
		for (j = 0; j< width; j++)
		{
			/*Calculate histogram if it is in the target block*/
			// promenio sam ovaj if, ovako je izgledao ranije: if (i>= tl_x && i < br_x && j >= tl_y && j < br_y )
			if (i>= tl_x && i <= br_x && j >= tl_y && j <= br_y )
				hist[(int)input_image[i*width + j]]++;
		}
	}
	//Stop timer for calculation of histogram
	PERF_END(PERFORMANCE_COUNTER_0_BASE,1);

	/* Cumulative histogram */

	//Measure time it takes for cumulative histogram to be calculated
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE,2);

	/*Cumulative histogram calculation loop*/
	cummhist[0] = hist[0];
	for (i = 1; i< 256; i++)
	{
		cummhist[i] = hist[i] + cummhist[i-1];
	}
	for (i = 0; i< 256; i++)
	{
		cummhist[i] = (255 * cummhist[i]) / roi_area;
	}
	//Stop timer for calculation of cumulative histogram
	PERF_END(PERFORMANCE_COUNTER_0_BASE,2);

	/* Image processing */

	//Measure time it takes for image processing to be done
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE,3);

	/*Image processing loop*/
	for (i = 0; i< height; i++)
	{
		for (j = 0; j< width; j++)
		{
			// ovo nam ne treba(jer treba da primenimo cummhist na celu ulaznu sliku bez obzira na region od interesa)
			//if (i>= tl_x && i < br_x && j >= tl_y && j < br_y )
			//	output_image[i*width + j] = (unsigned char)cummhist[input_image[i*width + j]];
			//else
			//	output_image[i*width + j] = input_image[i*width + j];
			//ovo nam samo treba:
			output_image[i*width + j] = (unsigned char)cummhist[input_image[i*width + j]];
		}
	}
	//Stop timer for image processing
	PERF_END(PERFORMANCE_COUNTER_0_BASE,3);

	//Print measured times
	perf_print_formatted_report(PERFORMANCE_COUNTER_0_BASE,alt_get_cpu_freq(),3, "histogram", "cumulative_hist", "image_processing");
	printf("\nProcessing done\n");

	/*Open output file and write result from output buffer to the output file*/
	fp = fopen("/mnt/host/low_contrast64_hist.bin", "wb");
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
