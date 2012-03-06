#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector> //for dynamically growing array
#include <map> //for key value pair of options
#include <algorithm> //for max_item()
#include <highgui.h>
#include <cv.h>

using namespace std;
typedef map<string, string> dictionary;

/* parse the file and return a,
 * dictionary of key, value pairs
 */
dictionary* parse(char** argc, const char token[] = ":"){

    dictionary *options = new dictionary;
    char each_line[200];
    char *key, *value;

    //open the specification file
    ifstream in;
    in.open(argc[1]);

    //on failure to open the file
    //display message and quit
    if( !in ){
	cout << "Could not read the specification file." << endl;
	exit(1);
    }

    //parse the file
    while( !in.eof()){

	//iterate over each line
	in.getline(each_line, 200);

	//tokenise the line into key and value pair
	key = strtok(each_line, token);
	value = strtok(NULL, token);

	//push to the dictonary
	//cout << key << " " << value << endl;
	if(key!=NULL and value!=NULL)
	    options->insert(pair<string, string>(string(key), string(value)));
    }

    in.close();

    return options;
}

/* If the file_name provided ends in .mask,
 * it fills in the pixels of the image from the values in the .mask,
 * otherwise it uses cvLoadImage to load the image file.
 *
 * save_file can be given a string, containing the path where
 * the generated image is to be stored
 */
IplImage* get_image(const string file_name, const string save_file = ""){
    
    IplImage *image;

    if(file_name.find(".mask")!=string::npos){

	//open the mask file
	ifstream in;
	in.open(file_name.c_str());

	//on failure to open the file
	//display message and quit
	if( !in ){
	    cout << "Could not read the image file." << endl;
	    exit(1);
	}

	int rows, columns, value;

	//first two entries of the mask file specify the number of rows and columns
	in >> columns;
	in >> rows;

	//create a blank image
	image = cvCreateImage(cvSize(columns, rows), IPL_DEPTH_8U, 1);

	//iterate over each pixel and fill it with correct value
	for(int row = 0; row < rows; row++){

	    //intialise pointers to the start of the row the image
	    uchar* ptr = (uchar*)(image->imageData + row*image->widthStep);

	    for( int column = 0; column < columns; column++ ){

		//read from the file the value of the pixel
		in >> value;
		if(value == 1)
		    ptr[column] = 255;
		else if(value == 0)
		    ptr[column] = 0;
		else
		    ptr[column] = 127;
	    }
	}

	//save the generated image to a file if a file name is provided
	if( !save_file.empty())
	    cvSaveImage(save_file.c_str(), image);
    }
    else{
	image = cvLoadImage(file_name.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    }

    return image;
}

/* Accepts an IplImage as a parameter,
 * creates a named window and displays the image,
 * quits when user presses <ESC>
 */
void show_image(IplImage *image){
    cvNamedWindow("Display", 1);
    cvShowImage("Display", image);
    while(1)
	if( cvWaitKey(10) == 27 )
	    break;
    cvDestroyWindow("Display");
}

/* Accepts source image and the kernel as IplImage
 * returns a pointer to the dilated image
 */
IplImage* dilate( const IplImage* src, const IplImage* kernel){

    //create a destination image with the same parametrs as the source image
    IplImage* dst = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);

    vector<uchar> pix_values;
    int i, j;

    //loop over each row
    for( int row = 0; row < src->height; row++ ){

	//intialise pointers to the start of the row of src and dst images
	uchar* dst_row_ptr = (uchar*)(dst->imageData + row*dst->widthStep);

	//loop over each column in the row
	for( int column = 0; column < src->width; column++ ){

	    pix_values.clear();

	    //loop over each row of the kernel
	    for( int krow = 0; krow < kernel->height; krow++){

		//initialise a temp pointer the the start of the row of src image
		i = row + krow - kernel->height/2;

		uchar* src_row_ptr = (uchar*)(src->imageData + i*src->widthStep);
		uchar* kernel_row_ptr = (uchar*)(kernel->imageData + krow*kernel->widthStep);

		//loop over each column of the kernel
		for( int kcolumn = 0; kcolumn < kernel->width; kcolumn++){

		    j = column + kcolumn - kernel->width/2;

		    if(i >= 0 && i < src->height && j >= 0 && j < src->width)
			if( kernel_row_ptr[kcolumn] == 255 )
			    pix_values.push_back(src_row_ptr[j]);
		}
	    }

	    dst_row_ptr[column] = *max_element(pix_values.begin(), pix_values.end());
	}
    }

    return dst;
}

/* Accepts source image and the kernel as IplImage
 * returns a pointer to the eroded image
 */
IplImage* erode(const IplImage* src, const IplImage* kernel){

    //create a destination image with the same parametrs as the source image
    IplImage* dst = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);

    vector<uchar> pix_values;
    int i, j;

    //loop over each row
    for( int row = 0; row < src->height; row++ ){

	//intialise pointers to the start of the row of src and dst images
	uchar* dst_row_ptr = (uchar*)(dst->imageData + row*dst->widthStep);

	//loop over each column in the row
	for( int column = 0; column < src->width; column++ ){

	    pix_values.clear();

	    //loop over each row of the kernel
	    for( int krow = 0; krow < kernel->height; krow++){

		//initialise a temp pointer the the start of the row of src image
		i = row + krow - kernel->height/2;

		uchar* src_row_ptr = (uchar*)(src->imageData + i*src->widthStep);
		uchar* kernel_row_ptr = (uchar*)(kernel->imageData + krow*kernel->widthStep);

		//loop over each column of the kernel
		for( int kcolumn = 0; kcolumn < kernel->width; kcolumn++){

		    j = column + kcolumn - kernel->width/2;

		    if(i >= 0 && i < src->height && j >= 0 && j < src->width)
			if( kernel_row_ptr[kcolumn] == 255 )
			    pix_values.push_back(src_row_ptr[j]);
		}
	    }

	    dst_row_ptr[column] = *min_element(pix_values.begin(), pix_values.end());
	}
    }

    return dst;
}

IplImage* opening(const IplImage *src, const IplImage *kernel){

    IplImage *tmp = cvCreateImage(cvSize(src->height, src->width), IPL_DEPTH_8U, 3 );
    IplImage *dst = cvCreateImage(cvSize(src->height, src->width), IPL_DEPTH_8U, 3 );
    tmp = erode(src, kernel);
    dst = dilate(tmp, kernel);
    cvReleaseImage(&tmp);
    return dst;
}

IplImage* closing(const IplImage *src, const IplImage *kernel){

    IplImage *tmp = cvCreateImage(cvSize(src->height, src->width), IPL_DEPTH_8U, 3 );
    IplImage *dst = cvCreateImage(cvSize(src->height, src->width), IPL_DEPTH_8U, 3 );
    tmp = dilate(src, kernel);
    dst = erode(tmp, kernel);
    cvReleaseImage(&tmp);
    return dst;
}

IplImage* hit_miss(const IplImage *src, const IplImage *kernel){

    IplImage *dst = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1 );
    bool match;
    int i, j;

    //loop over each row
    for( int row = 0; row < src->height ; row++ ){

	//intialise pointers to the start of the row of src and dst images
	uchar* dst_row_ptr = (uchar*)(dst->imageData + row*dst->widthStep);

	//loop over each column in the row
	for( int column = 0; column < src->width; column++ ){

	    match = true;

	    //loop over each row of the kernel
	    for( int krow = 0; krow < kernel->height && match; krow++){

		//initialise a temp pointer the the start of the row of src image
		i = row + krow - kernel->height/2;

		uchar* src_row_ptr = (uchar*)(src->imageData + i*src->widthStep);
		uchar* kernel_row_ptr = (uchar*)(kernel->imageData + krow*kernel->widthStep);

		//loop over each column of the kernel
		for( int kcolumn = 0; kcolumn < kernel->width && match; kcolumn++){

		    j = column + kcolumn - kernel->width/2;

		    if( kernel_row_ptr[kcolumn] != 127 ){
			if(  i >= 0 && i < src->height && j >= 0 && j < src->width){
			    if( kernel_row_ptr[kcolumn] != src_row_ptr[j])
				match = false;
			}
			else{
			    if( kernel_row_ptr[kcolumn] != 0)
				match = false;
			}
		    }
		}
	    }

	    if( match )
		dst_row_ptr[column] = 255;
	    else
		dst_row_ptr[column] = 0;
	}
    }

    return dst;
}

IplImage* boundary_extraction(const IplImage *src, const IplImage *kernel){
    IplImage *tmp = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1 );
    IplImage *dst = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1 );
    tmp = erode(src, kernel);
    cvSub(src, tmp, dst);
    cvReleaseImage(&tmp);
    return dst;
}

/* handles the mouse event for hole filling
 * sets seed to the point where the mouse event occoured
 */
void mouseHandler( int event, int x, int y, int flags, void *param){

    IplImage* dst = (IplImage*)param;
    switch( event ){

	//if the mouse event was a click
	case CV_EVENT_LBUTTONDOWN:
	    ((uchar *)(dst->imageData + y*dst->widthStep))[x] = 255;
	    break;
    }
}

/* Hole filling by selcting seed points.
 * When the image is displayed, select all the seed points
 * by clicking them. When done, pres <ESC>
 */
IplImage* hole_filling(const IplImage *src, const IplImage *kernel){

    IplImage *tmp;
    IplImage *dst = cvCreateImage( cvGetSize(src), IPL_DEPTH_8U, 1);
    cvZero(dst);

    //display the source image and attach the mouse listener
    cvNamedWindow("Hole", 1);
    cvSetMouseCallback("Hole", mouseHandler, (void*)dst );
    cvShowImage("Hole", src); 
    while( cvWaitKey(10) != 27 );
    cvDestroyWindow("Hole");

    for(int i = 1; i < 35; i++){
	tmp = dilate(dst, kernel);
	cvSub(tmp, src, dst);//use a second temp
    }

    cvMax(src, dst, dst);

    cvReleaseImage(&tmp);

    return dst;
}

IplImage* connected_component(const IplImage *src, const IplImage *kernel){

    IplImage *tmp;
    IplImage *dst = cvCreateImage( cvGetSize(src), IPL_DEPTH_8U, 1);
    cvZero(dst);

    //display the source image and attach the mouse listener
    cvNamedWindow("Hole", 1);
    cvSetMouseCallback("Hole", mouseHandler, (void*)dst );
    cvShowImage("Hole", src); 
    while( cvWaitKey(10) != 27 );
    cvDestroyWindow("Hole");

    for(int i = 1; i < 50; i++){
	tmp = dilate(dst, kernel);
	cvAnd(tmp, src, dst);//use a second temp
    }

    cvReleaseImage(&tmp);

    return dst;
}

IplImage* thinning(const IplImage *src, const IplImage *kernel){

    IplImage *dst = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);
    cvSub(src, hit_miss(src, kernel), dst);
    return dst;
}

IplImage* thickening(const IplImage *src, const IplImage *kernel){
    
    IplImage *dst = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);
    cvMax(src, hit_miss(src, kernel), dst);

    return dst;
}

IplImage* convex_hull(const IplImage *src){
    IplImage *kernel1 = get_image("kernels/ch_1.mask");
    IplImage *kernel2 = get_image("kernels/ch_2.mask");
    IplImage *kernel3 = get_image("kernels/ch_3.mask");
    IplImage *kernel4 = get_image("kernels/ch_4.mask");

    IplImage *res1 = cvCloneImage(src);
    for( int i =1; i <= 8; i++)
	cvMax(hit_miss(res1, kernel1), src, res1);
    IplImage *res2 = cvCloneImage(src);
    for( int i =1; i <= 8; i++)
	cvMax(hit_miss(res2, kernel2), src, res2);
    IplImage *res3 = cvCloneImage(src);
    for( int i =1; i <= 8; i++)
	cvMax(hit_miss(res3, kernel3), src, res3);
    IplImage *res4 = cvCloneImage(src);
    for( int i =1; i <= 8; i++)
	cvMax(hit_miss(res4, kernel4), src, res4);

    cvMax(res1, res2, res1);
    cvMax(res1, res3, res1);
    cvMax(res1, res4, res1);

    return res1;
}

IplImage* skeletonize(const IplImage *src, const IplImage *kernel){
}

IplImage* prune(const IplImage *src, const IplImage *kernel){
}

int main(int argv, char** argc){

    dictionary *options = parse(argc);

    //load and display the image
    string source = options->find(string("source"))->second;
    IplImage *src = get_image(source);
    show_image(src);

    //find the result
    IplImage *result = cvCloneImage(src);
    string task = options->find(string("task"))->second;

    if (task == "ER" ){
	IplImage *kernel = get_image((options->find(string("kernel"))->second).c_str());
	result = erode(src, kernel);
	cvReleaseImage(&kernel);
    }
    else if( task == "DL" ){
	IplImage *kernel = get_image((options->find(string("kernel"))->second).c_str());
	result = dilate(src, kernel);
	cvReleaseImage(&kernel);
    }
    else if( task == "OP" ){
	IplImage *kernel = get_image((options->find(string("kernel"))->second).c_str());
	result = opening(src, kernel);
	cvReleaseImage(&kernel);
    }
    else if( task == "CL" ){
	IplImage *kernel = get_image((options->find(string("kernel"))->second).c_str());
	result = closing(src, kernel);
	cvReleaseImage(&kernel);
    }
    else if( task == "HM" ){
	IplImage *kernel = get_image((options->find(string("kernel"))->second).c_str());
	result = hit_miss(src, kernel);
	cvReleaseImage(&kernel);
    }
    else if( task == "BE" ){
	IplImage *kernel = get_image((options->find(string("kernel"))->second).c_str());
	result = boundary_extraction(src, kernel);
	cvReleaseImage(&kernel);
    }
    else if( task == "HF" ){
	IplImage *kernel = get_image((options->find(string("kernel"))->second).c_str());
	result = hole_filling(src, kernel);
	cvReleaseImage(&kernel);
    }
    else if( task == "CC" ){
	IplImage *kernel = get_image((options->find(string("kernel"))->second).c_str());
	result = connected_component(src, kernel);
	cvReleaseImage(&kernel);
    }
    else if( task == "CH" ){
	result = convex_hull(src);
    }
    else if( task == "TN" ){
	IplImage *kernel1 = get_image("kernels/tn_1.mask");
	IplImage *kernel2 = get_image("kernels/tn_2.mask");
	IplImage *kernel3 = get_image("kernels/tn_3.mask");
	IplImage *kernel4 = get_image("kernels/tn_4.mask");
	IplImage *kernel5 = get_image("kernels/tn_5.mask");
	IplImage *kernel6 = get_image("kernels/tn_6.mask");
	IplImage *kernel7 = get_image("kernels/tn_7.mask");
	IplImage *kernel8 = get_image("kernels/tn_8.mask");

	result = thinning(result, kernel1);
	result = thinning(result, kernel2);
	result = thinning(result, kernel3);
	result = thinning(result, kernel4);
	result = thinning(result, kernel5);
	result = thinning(result, kernel6);
	result = thinning(result, kernel7);
	result = thinning(result, kernel8);

	cvReleaseImage(&kernel1);
	cvReleaseImage(&kernel2);
	cvReleaseImage(&kernel3);
	cvReleaseImage(&kernel4);
	cvReleaseImage(&kernel5);
	cvReleaseImage(&kernel6);
	cvReleaseImage(&kernel7);
	cvReleaseImage(&kernel8);
    }
    else if( task ==  "TK" ){
	IplImage *kernel1 = get_image("kernels/tk_1.mask");
	IplImage *kernel2 = get_image("kernels/tk_2.mask");
	IplImage *kernel3 = get_image("kernels/tk_3.mask");
	IplImage *kernel4 = get_image("kernels/tk_4.mask");
	IplImage *kernel5 = get_image("kernels/tk_5.mask");
	IplImage *kernel6 = get_image("kernels/tk_6.mask");
	IplImage *kernel7 = get_image("kernels/tk_7.mask");
	IplImage *kernel8 = get_image("kernels/tk_8.mask");

	result = thickening(result, kernel1);
	show_image(result);
	result = thickening(result, kernel2);
	show_image(result);
	result = thickening(result, kernel3);
	show_image(result);
	result = thickening(result, kernel4);
	show_image(result);
	result = thickening(result, kernel5);
	show_image(result);
	result = thickening(result, kernel6);
	show_image(result);
	result = thickening(result, kernel7);
	show_image(result);
	result = thickening(result, kernel8);
	show_image(result);

	cvReleaseImage(&kernel1);
	cvReleaseImage(&kernel2);
	cvReleaseImage(&kernel3);
	cvReleaseImage(&kernel4);
	cvReleaseImage(&kernel5);
	cvReleaseImage(&kernel6);
	cvReleaseImage(&kernel7);
	cvReleaseImage(&kernel8);
    }
    else if( task == "SK" ){
	IplImage *kernel = get_image((options->find(string("kernel"))->second).c_str());
	result = skeletonize(src, kernel);
	cvReleaseImage(&kernel);
    }
    else if( task == "PR" ){
	IplImage *kernel = get_image((options->find(string("kernel"))->second).c_str());
	result = prune(src, kernel);
	cvReleaseImage(&kernel);
    }

    //display and save the result
    show_image(result);
    string result_file = options->find(string("result"))->second;
    cvSaveImage(result_file.c_str(), result);

    //free the memory held by images and the kernel
    cvReleaseImage(&result);
    cvReleaseImage(&src);
}
