#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QTimer>
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>
#include <QPixmap>
#include <QImage>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#define TIMER_INTERVAL 500

using namespace cv;
using namespace std;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void paramsChanged();

private slots:
    void homomorfic();

    void on_btLoadImage_clicked();

private:
    Ui::MainWindow *ui;
    void shiftDFT(Mat& image);
    Mat image;
    QTimer * timer;
    bool loadImage(QString _file = "");
};

#endif // MAINWINDOW_H
