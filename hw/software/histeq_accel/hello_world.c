#include <stdio.h>
#include <stdlib.h>
#include <altera_avalon_performance_counter.h>
#include <system.h>
#include "altera_avalon_sgdma.h"
#include "altera_avalon_sgdma_regs.h"
#include "alt_types.h"
#include "sys/alt_cache.h"

#define HIST_LENGTH 256
void copy_callback_function(void * context)
{
	alt_u16 *cp_done = (alt_u16*) context;
	(*cp_done)++;  /* main will be polling for this value being 1 */
}
void transmit_callback_function(void * context)
{
	alt_u16 *tx_done = (alt_u16*) context;
	(*tx_done)++;  /* main will be polling for this value being 1 */
}

void receive_callback_function(void * context)
{
	alt_u16 *rx_done = (alt_u16*) context;
	(*rx_done)++;  /* main will be polling for this value being 1 */
}

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
    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE,1);

//	/* Histogram */
//
//	//Measure time it takes for histogram to be calculated
//    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE,1);
//
//	/*Histogram calculation loop*/
//	for (i = 0; i< height; i++)
//	{
//		for (j = 0; j< width; j++)
//		{
//			/*Calculate histogram if it is in the target block*/
//			// promenio sam ovaj if, ovako je izgledao ranije: if (i>= tl_x && i < br_x && j >= tl_y && j < br_y )
//			if (i>= tl_x && i <= br_x && j >= tl_y && j <= br_y )
//				hist[(int)input_image[i*width + j]]++;
//		}
//	}
//	//Stop timer for calculation of histogram
//	PERF_END(PERFORMANCE_COUNTER_0_BASE,1);
//
//	/* Cumulative histogram */
//
//	//Measure time it takes for cumulative histogram to be calculated
//	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE,2);
//
//	/*Cumulative histogram calculation loop*/
//	cummhist[0] = hist[0];
//	for (i = 1; i< 256; i++)
//	{
//		cummhist[i] = hist[i] + cummhist[i-1];
//	}
//	for (i = 0; i< 256; i++)
//	{
//		cummhist[i] = (255 * cummhist[i]) / roi_area;
//	}
//	//Stop timer for calculation of cumulative histogram
//	PERF_END(PERFORMANCE_COUNTER_0_BASE,2);
//
//	/* Image processing */
//
//	//Measure time it takes for image processing to be done
//	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE,3);
//
//	/*Image processing loop*/
//	for (i = 0; i< height; i++)
//	{
//		for (j = 0; j< width; j++)
//		{
//			// ovo nam ne treba(jer treba da primenimo cummhist na celu ulaznu sliku bez obzira na region od interesa)
//			//if (i>= tl_x && i < br_x && j >= tl_y && j < br_y )
//			//	output_image[i*width + j] = (unsigned char)cummhist[input_image[i*width + j]];
//			//else
//			//	output_image[i*width + j] = input_image[i*width + j];
//			//ovo nam samo treba:
//			output_image[i*width + j] = (unsigned char)cummhist[input_image[i*width + j]];
//		}
//	}
//	//Stop timer for image processing
//	PERF_END(PERFORMANCE_COUNTER_0_BASE,3);
//
//	//Print measured times
//	perf_print_formatted_report(PERFORMANCE_COUNTER_0_BASE,alt_get_cpu_freq(),3, "histogram", "cumulative_hist", "image_processing");
    alt_sgdma_dev * m2s_sgdma = alt_avalon_sgdma_open("/dev/sgdma_mm2s");
	alt_sgdma_dev * s2m_sgdma = alt_avalon_sgdma_open("/dev/sgdma_s2mm");
	if(m2s_sgdma == NULL)
		{
		  printf("Could not open the transmit SG-DMA\n");
		  return 1;
		}
	if(s2m_sgdma == NULL)
		{
		  printf("Could not open the transmit SG-DMA\n");
		  return 1;
	}
    volatile alt_u16 tx_done = 0;
    volatile alt_u16 rx_done = 0;
    alt_sgdma_descriptor *transmit_descriptor;
    alt_sgdma_descriptor *recieve_descriptor;
    int descriptor_length = roi_height;
	void * temp_ptr;
	temp_ptr = malloc((descriptor_length + 2) * ALTERA_AVALON_SGDMA_DESCRIPTOR_SIZE);
	if(temp_ptr == NULL)
	  {
		printf("Failed to allocate memory for the transmit descriptors\n");
		return 1;
	  }
	while((((alt_u32)temp_ptr) % ALTERA_AVALON_SGDMA_DESCRIPTOR_SIZE) != 0)
	  {
		temp_ptr++;  // slide the pointer until 32 byte boundary is found
	  }
	transmit_descriptor = (alt_sgdma_descriptor *)temp_ptr;
	transmit_descriptor[descriptor_length].control = 0;

	//void * temp_ptr;
	temp_ptr = malloc((1 + 2) * ALTERA_AVALON_SGDMA_DESCRIPTOR_SIZE);
	if(temp_ptr == NULL)
	  {
		printf("Failed to allocate memory for the transmit descriptors\n");
		return 1;
	  }
	while((((alt_u32)temp_ptr) % ALTERA_AVALON_SGDMA_DESCRIPTOR_SIZE) != 0)
	  {
		temp_ptr++;  // slide the pointer until 32 byte boundary is found
	  }
	recieve_descriptor = (alt_sgdma_descriptor *)temp_ptr;
	recieve_descriptor[1].control = 0;

	alt_u16 buffer_counter = 0;
	alt_u16 buffer_lengths = roi_width;
	for(buffer_counter = 0; buffer_counter < descriptor_length; buffer_counter++)
	{
	  /* This will create a descriptor that is capable of transmitting data from an Avalon-MM buffer
	   * to a packet enabled Avalon-ST FIFO component */
	  alt_avalon_sgdma_construct_mem_to_stream_desc(&transmit_descriptor[buffer_counter],  // current descriptor pointer
													&transmit_descriptor[buffer_counter+1], // next descriptor pointer
													(alt_u32*)&input_image[tl_y * width +  tl_x + width*buffer_counter],  // read buffer location
													(alt_u16)buffer_lengths,  // length of the buffer
													0, // reads are not from a fixed location
													0, // start of packet is disabled for the Avalon-ST interfaces
													0, // end of packet is disabled for the Avalon-ST interfaces,
													0);  // there is only one channel


	}
	  /* This will create a descriptor that is capable of transmitting data from an Avalon-ST FIFO
	   * to an Avalon-MM buffer */
	  alt_avalon_sgdma_construct_stream_to_mem_desc(&recieve_descriptor[0],  // current descriptor pointer
													&recieve_descriptor[1], // next descriptor pointer
													(alt_u32*)&hist,  // write buffer location
													(alt_u16)HIST_LENGTH,  // length of the buffer
													0); // writes are not to a fixed location
	alt_avalon_sgdma_register_callback(m2s_sgdma,
									&transmit_callback_function,
									(ALTERA_AVALON_SGDMA_CONTROL_IE_GLOBAL_MSK |
									 ALTERA_AVALON_SGDMA_CONTROL_IE_CHAIN_COMPLETED_MSK |
									 ALTERA_AVALON_SGDMA_CONTROL_PARK_MSK),
									(void*)&tx_done);

	alt_avalon_sgdma_register_callback(s2m_sgdma,
									&receive_callback_function,
									(ALTERA_AVALON_SGDMA_CONTROL_IE_GLOBAL_MSK |
									 ALTERA_AVALON_SGDMA_CONTROL_IE_CHAIN_COMPLETED_MSK |
									 ALTERA_AVALON_SGDMA_CONTROL_PARK_MSK),
								   (void*)&rx_done);
	if(alt_avalon_sgdma_do_async_transfer(m2s_sgdma, &transmit_descriptor[0]) != 0)
	{
	  printf("Writing the head of the transmit descriptor list to the DMA failed\n");
	  return 1;
	}
	if(alt_avalon_sgdma_do_async_transfer(s2m_sgdma, &recieve_descriptor[0]) != 0)
	{
	  printf("Writing the head of the receive descriptor list to the DMA failed\n");
	  return 1;
	}
	IOWR_32DIRECT(HISTEQ_CALC_0_BASE, 0, roi_area);
	IOWR_32DIRECT(HISTEQ_CALC_0_BASE, 1, 0x00000001);
    while(tx_done < 1) {}
    while(rx_done < 1) {}
    alt_avalon_sgdma_stop(m2s_sgdma);
    alt_avalon_sgdma_stop(s2m_sgdma);
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
	fp = fopen("/mnt/host/low_contrast64_histcalc.bin", "wb");
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
