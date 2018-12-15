#include <stdio.h>
#include <stdlib.h>
#include "altera_avalon_sgdma.h"
#include "altera_avalon_sgdma_regs.h"
#include "alt_types.h"
#include "system.h"
#include "sys/alt_cache.h"

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

	unsigned int width = 128;
	unsigned int height = 128;

	unsigned int tl_x, tl_y, br_x, br_y, roi_width, roi_height;

	/*Coordinates of top-left and bottom-right corner of the target block*/
	tl_x = 32; br_x = 96;
	tl_y = 32; br_y = 96;

	/*Width and height of the target block*/
	roi_height = br_x - tl_x + 1;
	roi_width = br_y - tl_y + 1;

	printf("Hello from Nios II!\n");

	/*Allocate memory space for input and ouput image buffers*/
	input_image = (unsigned char*) malloc(width*height);
	output_image = (unsigned char*) malloc(width*height);

	/*Open input file and read image into input buffer*/
	fp = fopen("/mnt/host/lena128.bin", "rb");
	fread(input_image, 1, width*height, fp);
	fclose(fp);

    printf("\nReading input done\n");

	/*Processing loop*/
//	for (i = 0; i< height; i++)
//	{
//		for (j = 0; j< width; j++)
//		{
//			/*Invert pixel if it is in the target block, else just copy unchanged value*/
//			if (i>= tl_x && i < br_x && j >= tl_y && j < br_y )
//				output_image[i*width + j] = 255 - input_image[i*width + j];
//			else
//				output_image[i*width + j] = input_image[i*width + j];
//
//		}
//	}
    /* otvaranje dma */
    alt_sgdma_dev * m2m_sgdma = alt_avalon_sgdma_open("/dev/m2m_sgdma");

    /* neke promenljive, ne trebaju nam */
    volatile alt_u16 cp_done = 0;

    alt_u32 return_code;
    alt_16 ** result_buffers;
    alt_8 ** data_buffers;

    if(m2m_sgdma == NULL)
    {
      printf("Could not open the transmit SG-DMA\n");
      return 1;
    }
    /*deskriptori deklaracija */
    alt_sgdma_descriptor *copy_descriptor;
    /* poravnavanje */
    void * temp_ptr;
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
    copy_descriptor = (alt_sgdma_descriptor *)temp_ptr;
    copy_descriptor[1].control = 0;
    /* kreiranje */
    alt_avalon_sgdma_construct_mem_to_mem_desc(&copy_descriptor[0],&copy_descriptor[1], input_image, output_image, (alt_u16)width * height, 0,0);
    /* prekidna rutina */
    alt_avalon_sgdma_register_callback(m2m_sgdma,
                                        &copy_callback_function,
                                        (ALTERA_AVALON_SGDMA_CONTROL_IE_GLOBAL_MSK |
                                         ALTERA_AVALON_SGDMA_CONTROL_IE_CHAIN_COMPLETED_MSK |
                                         ALTERA_AVALON_SGDMA_CONTROL_PARK_MSK),
                                        (void*)&cp_done);
    if(alt_avalon_sgdma_do_async_transfer(m2m_sgdma, &copy_descriptor[0]) != 0)
    {
      printf("Writing the head of the transmit descriptor list to the DMA failed\n");
      return 1;
    }
    /* otvaranje dma */
    alt_sgdma_dev * m2s_sgdma = alt_avalon_sgdma_open("/dev/m2s_sgdma");
    alt_sgdma_dev * s2m_sgdma = alt_avalon_sgdma_open("/dev/s2m_sgdma");
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
    /* neke promenljive, ne trebaju nam */
    volatile alt_u16 tx_done = 0;
    volatile alt_u16 rx_done = 0;

    /*deskriptori deklaracija */
    alt_sgdma_descriptor *transmit_descriptor;
    alt_sgdma_descriptor *recieve_descriptor;

    int descriptor_length = roi_height;
    //void * temp_ptr;
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
	recieve_descriptor = (alt_sgdma_descriptor *)temp_ptr;
	recieve_descriptor[descriptor_length].control = 0;

	alt_u16 buffer_counter = 0;
	alt_u16 buffer_lengths = roi_width;
	for(buffer_counter = 0; buffer_counter < descriptor_length; buffer_counter++)
	{
	  /* This will create a descriptor that is capable of transmitting data from an Avalon-MM buffer
	   * to a packet enabled Avalon-ST FIFO component */
	  alt_avalon_sgdma_construct_mem_to_stream_desc(&transmit_descriptor[buffer_counter],  // current descriptor pointer
													&transmit_descriptor[buffer_counter+1], // next descriptor pointer
													(alt_u32*)&input_image[(tl_y - 1) * width +  tl_x + width*buffer_counter],  // read buffer location
													(alt_u16)buffer_lengths,  // length of the buffer
													0, // reads are not from a fixed location
													0, // start of packet is disabled for the Avalon-ST interfaces
													0, // end of packet is disabled for the Avalon-ST interfaces,
													0);  // there is only one channel

	  /* This will create a descriptor that is capable of transmitting data from an Avalon-ST FIFO
	   * to an Avalon-MM buffer */
	  alt_avalon_sgdma_construct_stream_to_mem_desc(&recieve_descriptor[buffer_counter],  // current descriptor pointer
													&recieve_descriptor[buffer_counter+1], // next descriptor pointer
													(alt_u32*)&output_image[(tl_y - 1) * width +  tl_x + width*buffer_counter],  // write buffer location
													(alt_u16)buffer_lengths,  // length of the buffer
													0); // writes are not to a fixed location
	}


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

    while(cp_done < 1) {}
    cp_done = 0;
    alt_avalon_sgdma_stop(m2m_sgdma);

    /* Start non blocking transfer with DMA modules. */
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
  /**************************************************************/


  /**************************************************************
     * Blocking until the SGDMA interrupts fire                 *
     ************************************************************/
    while(tx_done < 1) {}
    while(rx_done < 1) {}

    tx_done = 0;
    rx_done = 0;

  /**************************************************************
     * Stop the SGDMAs                                          *
     ************************************************************/
    alt_avalon_sgdma_stop(m2s_sgdma);
    alt_avalon_sgdma_stop(s2m_sgdma);


	printf("\nProcessing done\n");

	/*Open output file and write result from output buffer to the output file*/
	fp = fopen("/mnt/host/lena128_neg.bin", "wb");
	fwrite(output_image, 1, width*height, fp);
	fclose(fp);

	printf("\nWriting output done\n");

	/*Clean memory*/
	free(input_image);
	free(output_image);

	printf("\nFinished\n");

	return 0;
}
