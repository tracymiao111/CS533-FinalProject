// University of Illinois, Urbana-Champaign
// Our SDK is based on Sitao Huang, Mang Yu & Ashutosh Dhar's template
// Demo code of LeNet Convolutional Neural Network
// *****************************************
// Before running the code make that the following files are on the SD card
// 1. images.bin
// 2. labels.bin
// 3. params.bin

#include <iostream>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <vector>
#include <array>
#include <atomic>

#include "xparameters.h"	/* SDK generated parameters */
#include "xsdps.h"			/* SD device driver */
#include "xil_printf.h"
#include "ff.h"				/* FAT File System */
#include "xil_cache.h"
#include "xplatform_info.h"
#include "platform.h"
#include "xtmrctr.h"
#include "xlenet_accelerator.h"

// Device declare
#define TMRCTR_DEVICE_ID      XPAR_AXI_TIMER_0_DEVICE_ID
#define TOPFUNCTION_DEVICE_ID XPAR_XLENET_ACCELERATOR_0_DEVICE_ID

// Max number of test cases in LeNet is 10K
#define NUM_TESTS 1000

// SD card and file reading objects
static FIL fil;		/* File object */
static FATFS fatfs;
static char *SD_File;
u32 Platform;

using namespace std;

// Static allocation of test images
unsigned char images[NUM_TESTS*28*28];
unsigned char labels[NUM_TESTS];
// *****************************************

// Static allocation of network parameters and their outputs
float image[1][32][32] = {0};
float conv1_weights[6][1][5][5] = {0};
float conv1_bias[6] = {0};
float conv1_output[6][28][28] = {0};

float pool2_output[6][14][14] = {0};

float conv3_weights[16][6][5][5] = {0};
float conv3_bias[16] = {0};
float conv3_output[16][10][10] = {0};

float pool4_output[16][5][5] = {0};

float conv5_weights[120][16][5][5] = {0};
float conv5_bias[120] = {0};
float conv5_output[120][1][1] = {0};

float fc6_weights[10][120][1][1] = {0};
float fc6_bias[10] = {0};
float fc6_output[10] = {0};
float fc6_output_hw[10] = {0};
// *****************************************
// End declaration of layers parameters and buffers
// *****************************************

// Start function definitions of different layers
inline float relu(float input)
{
    return (input > 0)? input:0;
}

// Convolution Layer 1
void convolution1(float input[1][32][32], float weights[6][1][5][5], float bias[6], float output[6][28][28])
{
    for(int co = 0; co < 6; co++)
        for(int h = 0; h < 28; h++)
            for(int w = 0; w < 28; w++)
            {
                float sum = 0;
                for(int i = h, m = 0; i < (h + 5); i++, m++)
                {
                    for(int j = w, n = 0; j < (w + 5); j++, n++)
                        sum += weights[co][0][m][n] * input[0][i][j];
                }
                output[co][h][w] = sum + bias[co];
            }
}

// Relu Layer 1
void relu1(float input[6][28][28], float output[6][28][28])
{
    for(int i = 0; i < 6; i++)
        for(int j = 0; j < 28; j++)
            for(int k = 0; k < 28; k++)
                output[i][j][k] = relu(input[i][j][k]);
}

// Pooling Layer 2
void max_pooling2(float input[6][28][28],float output[6][14][14])
{
    for(int c = 0;c < 6; c++)
        for(int h = 0; h < 14; h++)
            for(int w = 0; w < 14; w++)
            {
                float max_value=-1000000000000.0;
                for(int i = h*2; i < h*2+2; i++)
                {
                    for(int j = w*2;j < w*2+2; j++)
                        max_value = (max_value > input[c][i][j]) ? max_value:input[c][i][j];
                }
                output[c][h][w] = max_value;

            }
}

// Relu Layer 2
void relu2(float input[6][14][14], float output[6][14][14])
{
    for(int i = 0; i < 6; i++)
        for(int j = 0; j < 14; j++)
            for(int k = 0; k < 14; k++)
                output[i][j][k] = relu(input[i][j][k]);
}

// Convolution Layer 3
void convolution3(float input[6][14][14], float weights[16][6][5][5], float bias[16], float output[16][10][10])
{
    for(int co = 0; co < 16; co++)
        for(int h = 0; h < 10; h++)
            for(int w = 0; w < 10; w++)
            {
                    float sum = 0;
                    for(int i = h, m = 0; i < (h+5); i++, m++)
                    {
                        for(int j = w, n = 0; j < (w+5); j++, n++)
                            for (int ci = 0; ci < 6; ci++)
                                sum += weights[co][ci][m][n] * input[ci][i][j];
                    }
                    output[co][h][w] = sum + bias[co];
            }
}

// Relu Layer 3
void relu3(float input[16][10][10], float output[16][10][10])
{
    for(int i = 0; i < 16; i++)
        for(int j = 0; j < 10; j++)
            for(int k = 0; k < 10; k++)
                output[i][j][k] = relu(input[i][j][k]);
}

// Pooling Layer 4
void max_pooling4(float input[16][10][10],float output[16][5][5])
{
    for(int c = 0;c < 16; c++)
        for(int h = 0; h < 5; h++)
            for(int w = 0; w < 5; w++)
            {
                float max_value=-1000000000000.0;
                for(int i = h*2; i < h*2+2; i++)
                {
                    for(int j = w*2;j < w*2+2; j++)
                        max_value = (max_value > input[c][i][j]) ? max_value:input[c][i][j];
                }
                output[c][h][w] = max_value;
            }
}

// Relu Layer 4
void relu4(float input[16][5][5], float output[16][5][5])
{
    for(int i = 0; i < 16; i++)
        for(int j = 0; j < 5; j++)
            for(int k = 0; k < 5; k++)
                output[i][j][k] = relu(input[i][j][k]);
}

// Convolution Layer 5
void convolution5(float input[16][5][5], float weights[120][16][5][5], float bias[120], float output[120][1][1])
{
    for(int co = 0; co < 120; co++)
    {
        float sum = 0;
        for(int i = 0, m = 0; i < 5; i++, m++)
        {
            for(int j = 0, n = 0; j < 5; j++, n++)
            {
                for (int ci = 0; ci < 16; ci++)
                    sum += weights[co][ci][m][n] * input[ci][i][j];
            }
        }
        output[co][0][0] = sum + bias[co];
    }
}

// Relu Layer 5
void relu5(float input[120][1][1], float output[120][1][1])
{
    for(int i = 0; i < 120; i++)
        output[i][0][0] = relu(input[i][0][0]);
}

// Fully connected Layer 6
void fc6(const float input[120][1][1], const float weights[10][120][1][1], const float bias[10], float output[10])
{
    for(int n = 0; n < 10; n++)
    {
        output[n] = 0;
        for(int c = 0; c < 120; c++)
        {
            output[n] += weights[n][c][0][0] * input[c][0][0];
        }
        output[n]+=bias[n];
    }
}

// Relu Layer 6
void relu6(float input[10], float output[10])
{
    for(int i = 0; i < 10; i++)
        output[i] = relu(input[i]);
}
// *****************************************
// End declaration of layers functions
// *****************************************

XLenet_accelerator accelerator;
XTmrCtr timer;

int accelerator_init() {
	int status;

	//Init platform
	init_platform();
	xil_printf("Accelerator Demo Code\n\r");

	//Vector multiplier objects from xvector_multiplier.h

	XLenet_accelerator_Config *accelerator_config;
	// Get config data struct for vector mult
	accelerator_config = XLenet_accelerator_LookupConfig(TOPFUNCTION_DEVICE_ID);
	if(!accelerator_config)
	{
		xil_printf("No acc config found!\n\r");
		return 1;
	}
	else
	{
		xil_printf("Got acc config\n\r");
	}

	// Using config object to initialize the accelerator object
	status = XLenet_accelerator_CfgInitialize(&accelerator, accelerator_config);

	if(status != XST_SUCCESS)
	{
		xil_printf("Initialization of acc failed\n\r");
	}
	else
	{
		xil_printf("Initialization of acc successful\n\r");
	}

	//Initialize timer object
	status = XTmrCtr_Initialize(&timer, TMRCTR_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
		xil_printf("Timer init fail\n\r");
		return XST_FAILURE;
	}

	status = XTmrCtr_SelfTest(&timer, 0);
	if (status != XST_SUCCESS)
	{
		xil_printf("Timer self test fail\n\r");
		return XST_FAILURE;
	}

	XLenet_accelerator_Set_output_r_r(&accelerator, (u32)fc6_output_hw);

	XLenet_accelerator_InterruptGlobalEnable(&accelerator);
	Xil_DCacheFlush();
	XLenet_accelerator_InterruptEnable(&accelerator, 1);
	Xil_DCacheFlush();

	while (XLenet_accelerator_InterruptGetStatus(&accelerator) & 1 == 1) {
		XLenet_accelerator_InterruptClear(&accelerator, 1);
		Xil_DCacheFlush();
	}

	XLenet_accelerator_Set_w5_r(&accelerator, (u32)conv5_weights);

	Xil_DCacheFlush();
}

int sync_accelerator_run()
{
	//Start the accelerator
	XLenet_accelerator_Start(&accelerator);
	while (!XLenet_accelerator_IsDone(&accelerator));
	//Wait till the accelerator is not done
	Xil_DCacheFlush();

#ifdef DEBUG
	xil_printf("Accelerator Done\n\r");
#endif

	return 0;
}

// Parse MNIST test images
int parse_mnist_images(char* filename, unsigned char *images)
{
	FRESULT Res;
	UINT NumBytesRead;
    SD_File = (char *)filename;
    unsigned int header[4];

	Res = f_open(&fil, SD_File, FA_READ);
	if (Res)
	{
		xil_printf("ERROR when opening mnist images data file!\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Opened mnist images data file\n\r");
	}

	Res = f_lseek(&fil, 0);
	if (Res)
	{
		xil_printf("Cant seek to start\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Seeked to start\n\r");
	}

	Res = f_read(&fil, (void*)header, sizeof(unsigned int)*4, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read header from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read header from file\n\r");
	}

	Res = f_read(&fil, (void*)images, sizeof(unsigned char)*NUM_TESTS*28*28, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read images from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read images from file\n\r");
	}

	Res = f_close(&fil);
	if (Res)
	{
		xil_printf("Failed to close images file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Closed images file\n\r");
	}

	xil_printf("Returning...\n\r");


	return XST_SUCCESS;
}

// Parse MNIST test image labels
int parse_mnist_labels(char* filename, unsigned char *labels)
{
	FRESULT Res;
	UINT NumBytesRead;
    SD_File = (char *)filename;
    unsigned int header[2];

	Res = f_open(&fil, SD_File, FA_READ);
	if (Res)
	{
		xil_printf("ERROR when opening mnist label data file!\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Opened mnist labels data file\n\r");
	}

	Res = f_lseek(&fil, 0);
	if (Res)
	{
		xil_printf("Cant seek to start\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Seeked to start\n\r");
	}

	Res = f_read(&fil, (void*)header, sizeof(unsigned int)*2, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read header from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read header from file\n\r");
	}

	Res = f_read(&fil, (void*)labels, sizeof(unsigned char)*NUM_TESTS, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read labels from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read labels from file\n\r");
	}

	Res = f_close(&fil);
	if (Res)
	{
		xil_printf("Failed to close labels file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Closed labels file\n\r");
	}

	return 0;
}

// Parse parameter file and load it in to the arrays
int parse_parameters(char* filename)
{
	FRESULT Res;
	UINT NumBytesRead;
    SD_File = (char *)filename;

	Res = f_open(&fil, SD_File, FA_READ);
	if (Res)
	{
		xil_printf("ERROR when opening parameter file!\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Opened parameter file\n\r");
	}

	Res = f_lseek(&fil, 0);
	if (Res)
	{
		xil_printf("Cant seek to start\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Seeked to start\n\r");
	}

	Res = f_read(&fil, (void*)***conv1_weights, sizeof(float)*150, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read conv1_weights from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read conv1_weights from file\n\r");
	}

	Res = f_read(&fil, (void*)conv1_bias, sizeof(float)*6, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read conv1_bias from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read conv1_bias from file\n\r");
	}

	Res = f_read(&fil, (void*)***conv3_weights, sizeof(float)*2400, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read conv3_weights from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read conv3_weights from file\n\r");
	}

	Res = f_read(&fil, (void*)conv3_bias, sizeof(float)*16, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read conv3_bias from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read conv3_bias from file\n\r");
	}

	Res = f_read(&fil, (void*)***conv5_weights, sizeof(float)*48000, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read conv5_weights from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read conv5_weights from file\n\r");
	}

	Res = f_read(&fil, (void*)conv5_bias, sizeof(float)*120, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read conv5_bias from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read conv5_bias from file\n\r");
	}

	Res = f_read(&fil, (void*)***fc6_weights, sizeof(float)*1200, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read fc6_weights from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read fc6_weights from file\n\r");
	}

	Res = f_read(&fil, (void*)fc6_bias, sizeof(float)*10, &NumBytesRead);
	if (Res)
	{
		xil_printf("Cant read fc6_bias from file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Read fc6_bias from file\n\r");
	}

	Res = f_close(&fil);
	if (Res)
	{
		xil_printf("Failed to close labels file\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Closed labels file\n\r");
	}

	return 0;

}

// Fetch a single image to be processed.
//
void get_image(unsigned char *images, unsigned int idx, float image[1][32][32])
{
    for(int i = 0; i < 32; i++)
        for(int j = 0; j < 32; j++)
        {
            if (i < 2 || i > 29 || j < 2 || j > 29)
                image[0][i][j] = -1.0;
            else
                image[0][i][j] = images[idx*28*28 + (i-2)*28 + j-2] / 255.0 * 2.0 - 1.0;
        }
}

// single inference time
int main(int argc, char **argv)
{
	double total_hw_cycles = 0;
	double total_cpu_cycles = 0;
	cout<<"Starting LeNet\n\r";

	accelerator_init();

	cout<<"Mounting SD\n\r";
	FRESULT Res;
	TCHAR *Path = "0:/";
    Res = f_mount(&fatfs, Path, 0);
	if (Res != FR_OK)
	{
		xil_printf("Failed to open filesystem\n\r");
		return XST_FAILURE;
	}
	else
	{
		xil_printf("Mounted card\n\r");
	}

	cout<<"Parsing MNIST images\n\r";
	parse_mnist_images("images.bin", images);

	cout<<"Parsing MNIST labels\n\r";
	parse_mnist_labels("labels.bin", labels);

	cout<<"Parsing parameters\n\r";
	parse_parameters("params.bin");

	cout<<"Starting inference\n\r";
    int num_correct = 0;

    xil_printf("\n\rTest Image: ");

    for (int k = 0; k < NUM_TESTS; k++)
    {
    	//Get test image from dataset
        get_image(images, k, image);

        Xil_DCacheFlush();

        //Timer variables to measure time
		u32 timerCount_Stop;
		u32 timerCount_Start;

		//Reset the timer
		XTmrCtr_Reset(&timer, 0);
		XTmrCtr_ClearStats(&timer);
		XTmrCtr_Start(&timer, 0);
		//Start the timer
		timerCount_Start = XTmrCtr_GetValue(&timer, 0);

		//Begin inference here.
		convolution1(image, conv1_weights, conv1_bias, conv1_output);
		relu1(conv1_output, conv1_output);

		max_pooling2(conv1_output, pool2_output);
		relu2(pool2_output, pool2_output);

		convolution3(pool2_output, conv3_weights, conv3_bias, conv3_output);
		relu3(conv3_output, conv3_output);

		max_pooling4(conv3_output, pool4_output);
		relu4(pool4_output, pool4_output);

		convolution5(pool4_output, conv5_weights, conv5_bias, conv5_output);
		relu5(conv5_output, conv5_output);

		fc6(conv5_output, fc6_weights, fc6_bias, fc6_output);

		//Stop the timer since the accelerator is done
		timerCount_Stop = XTmrCtr_GetValue(&timer, 0);

		total_cpu_cycles += (timerCount_Stop - timerCount_Start) / 1000.0;

		//Reset the timer
		XTmrCtr_Reset(&timer, 0);
		XTmrCtr_ClearStats(&timer);
		XTmrCtr_Start(&timer, 0);
		//Start the timer
		timerCount_Start = XTmrCtr_GetValue(&timer, 0);

		//Set the offset address on the accelerator
		XLenet_accelerator_Set_input_r_r(&accelerator, (u32)image);

		Xil_DCacheFlush();

		//End inference here.
		sync_accelerator_run();

		//Stop the timer since the accelerator is done
		timerCount_Stop = XTmrCtr_GetValue(&timer, 0);

		total_hw_cycles += (timerCount_Stop - timerCount_Start) / 1000.0;

        //Check which output was largest.
        unsigned char result = 0;
        float p = -1000000.0;
        for(int i = 0; i < 10; i++)
        {
            if(fc6_output_hw[i] > p)
            {
                p = fc6_output_hw[i];
                result = i;
            }
        }
        //Largest output is result

        if(result == labels[k]) num_correct++;

        // std::cout << "test " << k << ": " << int(result) << "/" << int(labels[k]) << ": \n\r";

    }

    std::cout << "\n\rAccuracy = " << float(num_correct)/NUM_TESTS * 100.0 << "%" << std::endl;

    std::cout << "CPU Run " << total_cpu_cycles << " k cycles" << std::endl;
    std::cout << "HW Run " << total_hw_cycles << " k cycles" << std::endl ;

    cleanup_platform();
    return 0;
}
