#include <iostream>
#include <fstream>
#include <cv.h>
#include <highgui.h>
using namespace std;

void show_image(IplImage *image){
  cvNamedWindow("Display", 1);
  cvShowImage("Display", image);
  cvWaitKey(0);
  cvDestroyWindow("Display");
}

int main(int argc, char** argv){
    int row = atoi(argv[2]);
    int column = atoi(argv[3]);
    string shape(argv[4]);

    IplImage *kernel = cvCreateImage(cvSize(row, column), IPL_DEPTH_8U, 1);

    if( shape == "rectangle" ){
	for(int r = 0; r < row; r++){
	    unsigned char *ptr = (unsigned char*)(kernel->imageData + r*kernel->widthStep);
	    for( int c = 0; c < column; c++){
		ptr[c]=255;
	    }
	}
    }

    else if( shape == "cross" ){
	for(int r = 0; r < row; r++){
	    unsigned char *ptr = (unsigned char*)(kernel->imageData + r*kernel->widthStep);
	    for( int c = 0; c < column; c++){
		if( r == row/2 || c == column/2 )
		    ptr[c]=255;
		else
		    ptr[c]=0;
	    }
	}
    }

    else if( shape == "file" ){

	char each_line[100];

	ifstream file;
	file.open(argv[5]);
	
	//if failed to open the specification file
	if( !in ){
	    cout << "Could not read the specification file." << endl;
	    return 1;
	}

	in.getline(each_line, 100);
	int row = strtok(each_line, " "); 
	int column = strtok(NULL, token);

	//parse the file and collect all the key, value paris in a dictionary
	while( !in.eof()){

	    //iterate over each line
	    in.getline(each_line, 100);

	    for(int r = 0; r < row; r++){

		uchar *ptr = (uchar*)(kernel->imageData + kernel->widthStep);

		for( int c = 0; c < column; c++){
		}
	    }
	}
    }

    show_image(kernel);
    cvSaveImage( argv[1], kernel);
    cvReleaseImage(&kernel);
    return 0;
}
