#include <iostream>
#include <fstream>
#include <map>
#include <cv.h>
#include <highgui.h>

using namespace std;

void show_image(const IplImage *image){
    cvNamedWindow("Display", 1);
    cvShowImage("Display", image);
    while(1)
	if( cvWaitKey(10) == 27 )
	    break;
    cvDestroyWindow("Display");
}

IplImage* centering(const IplImage *src){

    IplImage *dst = cvCreateImage(cvGetSize(src), IPL_DEPTH_64F, 1);

    for( int r = 0; r < src->height; r++){

	double *src_row_ptr = (double*)(src->imageData + r*src->widthStep);
	double *dst_row_ptr = (double*)(dst->imageData + r*dst->widthStep);

	for(int c = 0; c < src->width; c++){
	    dst_row_ptr[c]=pow(-1.0, r+c)*src_row_ptr[c];
	}
    }

    return dst;
}

IplImage* get_dft(const IplImage *dft){

    IplImage *dst_real = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *dst_imaginary = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);

    cvSplit(dft, dst_real, dst_imaginary, 0, 0);

    // Compute the magnitude of the spectrum Mag = sqrt(Re^2 + Im^2)
    cvPow( dst_real, dst_real, 2.0);
    cvPow( dst_imaginary, dst_imaginary, 2.0);
    cvAdd( dst_real, dst_imaginary, dst_real, NULL);
    cvPow( dst_real, dst_real, 0.5 );

    // Compute log(1 + Mag)
    cvAddS( dst_real, cvScalarAll(1.0), dst_real, NULL ); // 1 + Mag
    cvLog( dst_real, dst_real ); // log(1 + Mag)

    double m, M;
    cvMinMaxLoc(dst_real, &m, &M, NULL, NULL, NULL);
    cvScale(dst_real, dst_real, 1.0/(M-m), 1.0*(-m)/(M-m));

    cvReleaseImage(&dst_imaginary);

    return dst_real;
}

IplImage* compute_output(const IplImage* filtered){

    IplImage *output_real = cvCreateImage(cvGetSize(filtered), IPL_DEPTH_64F, 1);
    IplImage *output_imaginary = cvCreateImage(cvGetSize(filtered), IPL_DEPTH_64F, 1);
    IplImage *output_complex = cvCreateImage(cvGetSize(filtered), IPL_DEPTH_64F, 2);

    // IDFT
    cvDFT(filtered, output_complex, CV_DXT_INV_SCALE);

    //split into real and imaginary component
    cvSplit(output_complex, output_real, output_imaginary, 0, 0);

    //convert the output to 8U depth
    IplImage *output = cvCreateImage(cvGetSize(filtered), IPL_DEPTH_8U, 1);
    IplImage *final = cvCreateImage( cvSize(output->width/2, output->height/2), IPL_DEPTH_8U, 1);
    cvScale(centering(output_real), output, 1);

    //get just the image
    CvMat mat;
    cvGetSubRect(output, &mat, cvRect(0, 0, output->width/2, output->height/2));
    cvCopy(&mat, final);

    cvReleaseImage(&output_real);
    cvReleaseImage(&output_imaginary);
    cvReleaseImage(&output_real);
    cvReleaseImage(&output);

    return final;
}


IplImage *compute_dft(const IplImage *src){

    //form a padded 64F depth, 2 channel source
    CvMat tmp;
    IplImage *src_real = cvCreateImage( cvGetSize(src), IPL_DEPTH_64F, 1);
    IplImage *src_imaginary = cvCreateImage( cvGetSize(src), IPL_DEPTH_64F, 1);
    IplImage *src_complex = cvCreateImage( cvGetSize(src), IPL_DEPTH_64F, 2);
    cvScale(src, src_real, 1.0, 0.0);
    cvZero(src_imaginary);
    cvMerge(centering(src_real), src_imaginary, NULL, NULL, src_complex);

    //copy src_imaginary to dft and pad it
    //and find the dft
    IplImage *dft = cvCreateImage( cvSize(2*src->width, 2*src->height), IPL_DEPTH_64F, 2);
    cvGetSubRect(dft, &tmp, cvRect(0, 0, src->width, src->height));
    cvCopy(src_complex, &tmp);
    cvGetSubRect(dft, &tmp, cvRect(src->width, 0, src->width, src->height));
    cvZero(&tmp);
    //cvGetSubRect(dft, &tmp, cvRect(0, src->height, dft->width, src->height));
    //cvZero(&tmp);
    cvDFT(dft, dft, CV_DXT_FORWARD, src->height);

    //cleanup
    cvReleaseImage(&src_real);
    cvReleaseImage(&src_imaginary);
    cvReleaseImage(&src_complex);

    return dft;
}

IplImage* ideal_low_pass(const IplImage *dft, int cutoff){

    IplImage *filter_real = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_imaginary = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_complex = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);
    IplImage *multiply = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);

    //get filter_real
    for( int r = 0; r < dft->height; r++){

	double *ptr = (double*)(filter_real->imageData + r*filter_real->widthStep);

	for( int c = 0; c < dft->width; c++){
	    double d = pow(pow(r - dft->height/2, 2) + pow(c - dft->width/2, 2), 0.5);
	    ptr[c] = (d < cutoff)? 1 : 0;
	}
    }
    cvZero(filter_imaginary);
    cvMerge(filter_real, filter_imaginary, NULL, NULL, filter_complex);

    //multiply the signals
    cvMulSpectrums(dft, filter_complex, multiply, CV_DXT_FORWARD);

    cvReleaseImage(&filter_real);
    cvReleaseImage(&filter_imaginary);
    cvReleaseImage(&filter_complex);

    return multiply;
}

IplImage* ideal_high_pass(const IplImage *dft, const int cutoff){

    IplImage *filter_real = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_imaginary = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_complex = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);
    IplImage *multiply = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);

    //get filter_real
    for( int r = 0; r < dft->height; r++){

	double *ptr = (double*)(filter_real->imageData + r*filter_real->widthStep);

	for( int c = 0; c < dft->width; c++){
	    double d = pow(pow(r - dft->height/2, 2) + pow(c - dft->width/2, 2), 0.5);
	    ptr[c] = (d < cutoff)? 0 : 1;
	}
    }
    cvZero(filter_imaginary);
    cvMerge(filter_real, filter_imaginary, NULL, NULL, filter_complex);

    //multiply the signals
    cvMulSpectrums(dft, filter_complex, multiply, CV_DXT_FORWARD);

    cvReleaseImage(&filter_real);
    cvReleaseImage(&filter_imaginary);
    cvReleaseImage(&filter_complex);

    return multiply;
}

IplImage* butterworth_low_pass(IplImage *dft, const int cutoff, const int order){

    IplImage *filter_real = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_imaginary = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_complex = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);
    IplImage *multiply = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);

    //get filter_real
    for( int r = 0; r < dft->height; r++){

	double *ptr = (double*)(filter_real->imageData + r*filter_real->widthStep);

	for( int c = 0; c < dft->width; c++){
	    double d = pow(pow(r - dft->height/2, 2) + pow(c - dft->width/2, 2), 0.5);
	    ptr[c] = 1/(1 + pow(d/cutoff, 2*order));
	}
    }
    cvZero(filter_imaginary);
    cvMerge(filter_real, filter_imaginary, NULL, NULL, filter_complex);
    show_image(get_dft(filter_complex));

    //multiply the signals
    cvMulSpectrums(dft, filter_complex, multiply, CV_DXT_FORWARD);

    cvReleaseImage(&filter_real);
    cvReleaseImage(&filter_imaginary);
    cvReleaseImage(&filter_complex);

    return multiply;
}

IplImage* butterworth_high_pass(const IplImage *dft, const int cutoff, const int order){

    IplImage *filter_real = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_imaginary = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_complex = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);
    IplImage *multiply = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);

    //get filter_real
    for( int r = 0; r < dft->height; r++){

	double *ptr = (double*)(filter_real->imageData + r*filter_real->widthStep);

	for( int c = 0; c < dft->width; c++){
	    double d = pow(pow(r - dft->height/2, 2) + pow(c - dft->width/2, 2), 0.5);
	    ptr[c] = 1/(1 + pow(cutoff/d, 2*order));
	}
    }
    cvZero(filter_imaginary);
    cvMerge(filter_real, filter_imaginary, NULL, NULL, filter_complex);

    //multiply the signals
    cvMulSpectrums(dft, filter_complex, multiply, CV_DXT_FORWARD);

    cvReleaseImage(&filter_real);
    cvReleaseImage(&filter_imaginary);
    cvReleaseImage(&filter_complex);

    return multiply;
}

IplImage* laplacian(const IplImage *dft){

    IplImage *filter_real = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_imaginary = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_complex = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);
    IplImage *multiply = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);

    //get filter_real
    for( int r = 0; r < dft->height; r++){

	double *ptr = (double*)(filter_real->imageData + r*filter_real->widthStep);

	for( int c = 0; c < dft->width; c++){
	    double d = pow(r - dft->height/2, 2) + pow(c - dft->width/2, 2);
	    ptr[c] = -4*3.14*3.14*d;
	}
    }
    cvZero(filter_imaginary);
    cvMerge(filter_real, filter_imaginary, NULL, NULL, filter_complex);
    show_image(get_dft(filter_complex));

    //multiply the signals
    cvMulSpectrums(dft, filter_complex, multiply, CV_DXT_FORWARD);

    cvReleaseImage(&filter_real);
    cvReleaseImage(&filter_imaginary);
    cvReleaseImage(&filter_complex);

    return multiply;
}

IplImage* band_reject(const IplImage *dft, const int cutoff, const int width){

    IplImage *filter_real = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_imaginary = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
    IplImage *filter_complex = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);
    IplImage *multiply = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);

    //get filter_real
    for( int r = 0; r < dft->height; r++){

	double *ptr = (double*)(filter_real->imageData + r*filter_real->widthStep);

	for( int c = 0; c < dft->width; c++){
	    double d = pow(pow(r - dft->height/2, 2) + pow(c - dft->width/2, 2), 0.5);
	    ptr[c] = (((cutoff - width/2) <= d) && (d <= (cutoff + width/2)))? 0 : 1;
	}
    }
    show_image(filter_real);
    cvZero(filter_imaginary);
    cvMerge(filter_real, filter_imaginary, NULL, NULL, filter_complex);

    //multiply the signals
    cvMulSpectrums(dft, filter_complex, multiply, CV_DXT_FORWARD);

    cvReleaseImage(&filter_real);
    cvReleaseImage(&filter_imaginary);
    cvReleaseImage(&filter_complex);

    return multiply;
}

void mouseHandler( int event, int x, int y, int flags, void *param){

    switch( event ){

	case CV_EVENT_LBUTTONDOWN:
	    cvDestroyWindow("Notch");
	    IplImage *dft = (IplImage*) param;
	    IplImage *filter_real = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
	    IplImage *filter_imaginary = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 1);
	    IplImage *filter_complex = cvCreateImage( cvGetSize(dft), IPL_DEPTH_64F, 2);

	    //get filter_real
	    for( int r = 0; r < dft->height; r++){

		double *ptr = (double*)(filter_real->imageData + r*filter_real->widthStep);

		for( int c = 0; c < dft->width; c++){
		    double d1 = pow(pow(r - dft->height/2 - y, 2) + pow(c - dft->width/2 - x, 2), 0.5);
		    double d2 = pow(pow(r - dft->height/2 + y, 2) + pow(c - dft->width/2 + x, 2), 0.5);
		    ptr[c] = 1/(1 + pow(50/d, 2*2));
		}
	    }
	    cvZero(filter_imaginary);
	    cvMerge(filter_real, filter_imaginary, NULL, NULL, filter_complex);

	    show_image(get_dft(filter_complex));
	    //multiply the signals
	    cvMulSpectrums(dft, filter_complex, dft, CV_DXT_FORWARD);

	    cvReleaseImage(&filter_real);
	    cvReleaseImage(&filter_imaginary);
	    cvReleaseImage(&filter_complex);
	    break;
    }
}

IplImage* butterworth_notch(IplImage *dft, const int cutoff, const int order){

    IplImage *result = cvCloneImage(dft);
    IplImage *display = get_dft(dft);

    cvNamedWindow("Notch", 1);
    cvSetMouseCallback("Notch", mouseHandler, (void*)result );
    cvShowImage("Notch", display); 
    while( cvWaitKey(10) != 27 );

    return result;
}

int main(int argv, char **argc){

    char each_line[100], *key, *value;
    char token[] = ": ";
    map<string, string> dict;

    //open the specification file
    ifstream in;
    in.open(argc[1]);

    //if failed to open the specification file
    if( !in ){
	cout << "Could not read the specification file." << endl;
	return 1;
    }

    //parse the file and collect all the key, value paris in a dictionary
    while( !in.eof()){

	//iterate over each line
	in.getline(each_line, 100);

	//tokenise the line into key and value pair
	key = strtok(each_line, token);
	value = strtok(NULL, token);

	//push to the dictonary
	if(key != NULL and value != NULL)
	    dict.insert(pair<string, string>(string(key), string(value)));
    }

    in.close();

    cout << dict.find(string("source"))->second << endl;

    //load src 
    IplImage *src = cvLoadImage((dict.find(string("source"))->second).c_str(), CV_LOAD_IMAGE_ANYCOLOR);
    show_image(src);

    //find and display the dft
    IplImage *dft = compute_dft(src);
    //show_image(get_dft(dft));

    //compute the filtered output
    IplImage *filtered;
    string filter = dict.find(string("filter"))->second;

    if( filter == "ILP" ){
	int cutoff = atoi((dict.find(string("cutoff"))->second).c_str());
	filtered = ideal_low_pass(dft, cutoff);
    }
    else if( filter == "IHP" ){
	int cutoff = atoi((dict.find(string("cutoff"))->second).c_str());
	filtered = ideal_high_pass(dft, cutoff);
    }
    else if( filter == "BLP" ){
	int cutoff = atoi((dict.find(string("cutoff"))->second).c_str());
	int order = atoi((dict.find(string("order"))->second).c_str());
	filtered = butterworth_low_pass(dft, cutoff, order);
    }
    else if( filter == "BHP" ){
	int cutoff = atoi((dict.find(string("cutoff"))->second).c_str());
	int order = atoi((dict.find(string("order"))->second).c_str());
	filtered = butterworth_high_pass(dft, cutoff, order);
    }
    else if( filter == "LAP" ){
	filtered = laplacian(dft);
    }
    else if( filter == "BRF" ){
	int cutoff = atoi((dict.find(string("cutoff"))->second).c_str());
	int width = atoi((dict.find(string("width"))->second).c_str());
	filtered = band_reject(dft, cutoff, width);
    }
    else if( filter == "BNF" ){
	int cutoff = atoi((dict.find(string("cutoff"))->second).c_str());
	int order = atoi((dict.find(string("order"))->second).c_str());
	filtered = butterworth_notch(dft, cutoff, order);
    }
	
    show_image(get_dft(filtered));
    IplImage *dst = cvCreateImage( cvGetSize( filtered), IPL_DEPTH_8U, 1);
    dst = compute_output(filtered);
    show_image(dst);
    cvSaveImage((dict.find(string("output"))->second).c_str(), dst);

    cvReleaseImage(&src);
    cvReleaseImage(&dft);
    //cvReleaseImage(&filtered);
}
