#include <iostream>
#include "../include/Convolution_layer.h"
#include<vector>
#include<omp.h> 

Convolution_layer::Convolution_layer() {} //constructor


/*Here, we will update the weights of the filters using the Backpropagation

Hidden contain the hidden layers in the backward phase, crucial for caching:
(1) the input from convolution layer "before flattening" it, (2) the input "after flattening" and (3) the values input of the "softmax activation function" */

void Convolution_layer::Hidden(const std::vector<double>& vect) { //caching

    HiddenMat.clear(); //Clear the old HiddenMat
    HiddenMat.resize(vect.size()); //Resize HiddenMat

    //Copy: output.assign(input.begin(), input.end());
    HiddenMat.assign(vect.begin(), vect.end()); //output = HiddenMat
}


//in convolution we use the whole volume of the input matrix n*n*channels(RGB, 3)
void Convolution_layer::convolution_parameters(const std::vector<double>& vec_pixel, int inputImage_height, int inputImage_width) {

    size_t ii,jj;
  
    ConvMat_height = ((inputImage_height - filter_height + 2 * padding) / stride) + 1; //output convolution matrix height
    ConvMat_width = ((inputImage_width - filter_width + 2 * padding) / stride) + 1; //output convolution matrix width

    //initialization of the filter weights by random values (class Random_weights)
    if (initialization) {
        random_weights(filter_number, filter_height * filter_width, filter_matrix); //initialization of weights with random values

        //normalizing wight values, Ref3
        #pragma omp parallel for private(jj)
        for ( ii = 0; ii < filter_number; ii++) { //loop on number of filters
            for (jj = 0; jj < (filter_height * filter_width); jj++) { //loop on total number of weights within each filter
                filter_matrix[ii][jj] = (double)filter_matrix[ii][jj] / (double)(filter_height * filter_width); //normalized filter
            }
        }
        initialization = false;
    }

    ConvMat.clear();

    //Convolution procedure for filter_number
    convolution_process(vec_pixel, 0); //1st filter
    convolution_process(vec_pixel, 1); //2sd filter
    convolution_process(vec_pixel, 2); //3rd filter
    convolution_process(vec_pixel, 3); //4th filter
    convolution_process(vec_pixel, 4); //5th filter
    convolution_process(vec_pixel, 5); //6th filter
    convolution_process(vec_pixel, 6); //7th filter
    convolution_process(vec_pixel, 7); //8th filter

    Hidden(vec_pixel); //hiding the last input
}

void Convolution_layer::convolution_process(const std::vector<double>& pixel, int idx) {
    std::vector<double> vec;
    int Tile=16;
    // #pragma unroll(16)    
    // for (int ii = 0; ii < ConvMat_height; ii++) { //loop on the height of the convolution matrix
       for (int a = 0; a < ConvMat_height; a+=Tile) {
	 //#pragma unroll(16) 
        for (int jj = 0; jj < ConvMat_width; jj++) { //loop on the width of the convolution matrix
	  for(int ii = a; ii< a+Tile; ii++){
            double sum = 0; //initialization of the summation
           
            for (int kk = 0; kk < filter_height; kk++) { //loop on the height of the filter
                for (int hh = 0; hh < filter_width; hh++) { //loop on the width of the filter

                    double image = (pixel[((ii + kk) * (ConvMat_width + 2) + (jj + hh))]); //pixel value of the image stored in image

                    sum += (image * filter_matrix[idx][kk * filter_width + hh]); //sum of the pixel value * filter value

                }
            }
            vec.push_back(sum); //storing sum value in vec
	  }
        }
    }
    ConvMat.push_back(vec); //storing vec value in ConvMat 
}

void Convolution_layer::BackPropagation(std::vector<std::vector<double>> d_L_d_out, double learn_rate)
{
    //d_L_d_out is the loss gradient for this layer's outputs
    //filters with same shape as filter_matrix
    std::vector<std::vector<double>> filters;
    #pragma unroll(16) 
    for (size_t i = 0; i < filter_number; i++) {
        std::vector<double> v; 
	for (int j = 0; j < (filter_height * filter_width); j++)
            v.push_back(0);
        filters.push_back(v);
    }
   
    //For keeping 3x3 reegions of last input
    std::vector<std::vector<double>> regions;

    //Loop for storing 3x3 regions into "regions"
    #pragma unroll(16)
    for (int i = 0; i < ConvMat_height; i++)
    {
        for (int j = 0; j < ConvMat_width; j++) {
            //double sum = 0;
            std::vector<double> v;
            for (int k = 0; k < filter_height; k++) {
                for (int n = 0; n < filter_width; n++) {
                    v.push_back(HiddenMat[((i + k) * (ConvMat_width + 2) + (j + n))]);
                }
            }
            regions.push_back(v);
        }
    }

    //Loop for iterating d_L_d_out(last output of this layer)
    int counter = 0;
    #pragma omp for schedule(dynamic)
    for (int i = 0; i < ConvMat_height; i++) {
        for (int j = 0; j < ConvMat_width; j++) {
            //Loop for filters number
            for (size_t k = 0; k < 2; k++) {
                for (size_t m = 0; m < 3; m++) {
                    for (size_t n = 0; n < 3; n++) {
                        filters[k][m * 3 + n] += ((d_L_d_out[k][i * ConvMat_width + j] * regions[counter][m * 3 + n]));
                    }
                }
            }
            counter++;
        }
    }
    #pragma omp for schedule(dynamic)
    for (size_t i = 0; i < filter_number; i++) {
        for (size_t j = 0; j < 3; j++) {
            for (size_t k = 0; k < 3; k++) {
                filter_matrix[i][j * 3 + k] -= (learn_rate * filters[i][j * 3 + k]);
            }
        }
    }
}
void Convolution_layer::random_weights(double nb_filters, double nb_weights, std::vector<std::vector<double>>& filter_matrix) {

    //construct a random generator engine from a time-based seed, Ref1
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count(); //time; system real time

    std::default_random_engine generator(seed); //random generator engine
    std::normal_distribution<double> distribution(0.0, 1.0); //( result_type mean = 0.0, result_type stddev = 1.0 )

    #pragma unroll(16)
    for (int ii = 0; ii < nb_filters; ii++) { //loop on the total number of filters

        std::vector<double> one_filter; //array initialisation with no defined size

        for (int jj = 0; jj < nb_weights; jj++) { //nb_weights = filter height * filter width

            double number = (distribution(generator)); //random number from a random generator engine, Ref2

            one_filter.push_back(number); // Filling of 1 filter with random values

        }

        filter_matrix.push_back(one_filter); //Filling of all filters with random values
    }

}
Convolution_layer::~Convolution_layer()
{

}
/*	References:

    Ref1: https://www.cplusplus.com/reference/random/normal_distribution/normal_distirbution/

    Ref2: https://www.cplusplus.com/reference/random/normal_distribution/

 */
