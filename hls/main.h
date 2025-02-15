void convolution1(float input[1][32][32], float weights[2][1][5][5], float bias[2], float output[2][28][28]);
void relu1(float input[2][28][28], float output[2][28][28]);
void max_pooling2(float input[2][28][28],float output[2][14][14]);
void relu2(float input[2][14][14], float output[2][14][14]);
void convolution3(float input[2][14][14], float weights[5][2][5][5], float bias[5], float output[5][10][10]);
void relu3(float input[5][10][10], float output[5][10][10]);
void max_pooling4(float input[5][10][10],float output[5][5][5]);
void relu4(float input[5][5][5], float output[5][5][5]);
void convolution5(float input[5][5][5], unsigned char weights[120][5][5][5], float bias[120], float output[120][1][1], float q_scale[120][5], float q_bias[120][5]);
void relu5(float input[120][1][1], float output[120][1][1]);
void fc6(const float input[120][1][1], const float weights[10][120][1][1], const float bias[10], float output[10]);
void relu6(float input[10], float output[10]);

int lenet_accelerator(float input[1][32][32], float output[10]);
