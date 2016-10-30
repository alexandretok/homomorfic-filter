#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    connect(ui->lowFreqSlider, SIGNAL(sliderMoved(int)), this,  SLOT(paramsChanged()));
    connect(ui->highFreqSlider, SIGNAL(sliderMoved(int)), this,  SLOT(paramsChanged()));
    connect(ui->cSlider, SIGNAL(sliderMoved(int)), this,  SLOT(paramsChanged()));
    connect(ui->d0Slider, SIGNAL(sliderMoved(int)), this,  SLOT(paramsChanged()));

    //-- This timer makes sure we don't call the `homomorfic()` function all the time
    timer = new QTimer(this);
    timer->setInterval(TIMER_INTERVAL);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(homomorfic()));
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::paramsChanged(){
    //-- This gets called everytime a slider value gets changed
    ui->labelLowFreq->setText(QString::number((float)ui->lowFreqSlider->value()/10));
    ui->labelHighFreq->setText(QString::number((float)ui->highFreqSlider->value()/10));
    ui->labelC->setText(QString::number((float)ui->cSlider->value()/10));
    ui->labelD0->setText(QString::number((float)ui->d0Slider->value()));

    if(image.data){
        if(timer->isActive())
            //-- If it is active, resets the time to start counting again
            timer->setInterval(TIMER_INTERVAL);
        else
            timer->start();
    }else
        ui->statusBar->showMessage("Select an image to apply the filter");
}

void MainWindow::homomorfic(){
    Mat complexImage, padded, filter, tmp;
    Mat_<float> realInput, zeros;
    vector<Mat> planes;

    if(!image.data)
        return;

    //-- Identifies the best sizes to the FFT calculation
    int dft_M = getOptimalDFTSize(image.rows),
        dft_N = getOptimalDFTSize(image.cols);

    //-- Padding the image
    copyMakeBorder(image, padded, 0,
                dft_M - image.rows, 0,
                dft_N - image.cols,
                BORDER_CONSTANT, Scalar::all(0));

    //-- The imaginary part of the complex matrix (padded with zeros)
    zeros = Mat_<float>::zeros(padded.size());

    //-- Prepares the complex matrix to be filled
    complexImage = Mat(padded.size(), CV_32FC2, Scalar(0));

    //-- The tranfer function (frequency filter) should
    //-- have the same size as the complex matrix
    filter = complexImage.clone();

    //-- Creates a temporary matrix to create the real and imaginary filter components
    tmp = Mat(dft_M, dft_N, CV_32F);

    float highFreq = (float)ui->highFreqSlider->value()/10,
    lowFreq = (float)ui->lowFreqSlider->value()/10,
    cParam  = (float)ui->cSlider->value()/10,
    d0 = ui->d0Slider->value();

    int M = dft_M, N = dft_N;

    //-- Prepares the homomorphic filter
    for(int i=0; i<dft_M; i++){
        for(int j=0; j<dft_N; j++){
            tmp.at<float> (i,j) = (highFreq-lowFreq)*(1.0-exp(-1.0*(float)cParam*((((float)i-M/2.0)*((float)i-M/2.0) + ((float)j-N/2.0)*((float)j-N/2.0))/(d0*d0))))+ lowFreq;
        }
    }

    //-- Creates the matrix with the filter componentes and
    //-- merges both in a multi channel complex matrix
    Mat comps[]= {tmp, tmp};
    merge(comps, 2, filter);

    //-- Clears the matrix array that are going to hold the complex image
    planes.clear();
    //-- Creates the real components
    realInput = Mat_<float>(padded);
    //-- Inserts the two components into the matrix array
    planes.push_back(realInput);
    planes.push_back(zeros);

    //-- Combines the matrix array into one complex component
    merge(planes, complexImage);

    //-- DFT calculation
    dft(complexImage, complexImage);

    //-- Shifts the quadrants
    shiftDFT(complexImage);

    //-- Applies the frequency filter
    mulSpectrums(complexImage,filter,complexImage,0);

    //-- Clears the planes array
    planes.clear();
    //-- Splits the real and imaginary parts to modify them
    split(complexImage, planes);

    //-- Recomposes the planes into one complex matrix
    merge(planes, complexImage);

    //-- Shift the quadrants back to normal
    shiftDFT(complexImage);

    //-- Inverse DFT calculation
    idft(complexImage, complexImage);

    //-- Clears the planes array
    planes.clear();

    //-- Separates the real and imaginary parts of the filtered image
    split(complexImage, planes);

    //-- Normalizes the real part to exhibition
    normalize(planes[0], planes[0], 0, 1, CV_MINMAX);

    imshow("original", image);
    imshow("filtrada", planes[0]);
}

bool MainWindow::loadImage(QString _file){
    //-- If file path was passed uses it, otherwise, opens file selection dialog
    QString file = _file != "" ? _file : QFileDialog::getOpenFileName(this, tr("Select Image"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0), tr("Image File (*.png);;All Files (*)"));

    if(file.isEmpty())
        return false;

    image = imread(file.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);

    if(image.data)
        return true;
    else
        return false;
}

void MainWindow::shiftDFT(Mat& image){
    Mat tmp, A, B, C, D;

    //-- If the image has a odd size, crops it to avoid different size copies
    image = image(Rect(0, 0, image.cols & -2, image.rows & -2));
    int cx = image.cols/2;
    int cy = image.rows/2;

    //-- Reorganizes transform quadrants
    //-- A B -> D C
    //-- C D -> B A
    A = image(Rect(0, 0, cx, cy));
    B = image(Rect(cx, 0, cx, cy));
    C = image(Rect(0, cy, cx, cy));
    D = image(Rect(cx, cy, cx, cy));

    //-- A <-> D
    A.copyTo(tmp);  D.copyTo(A);  tmp.copyTo(D);

    //-- C <-> B
    C.copyTo(tmp);  B.copyTo(C);  tmp.copyTo(B);
}

void MainWindow::on_btLoadImage_clicked(){
    loadImage("/home/alexandre/projetos/pdi/homomorphic-filter/IMG_20161030_195746.png");
    homomorfic();
}
