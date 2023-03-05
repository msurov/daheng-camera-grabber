#include <string>
#include <iostream>
#include "gxcam.h"

bool callback(cv::Mat const& raw)
{
    std::cout << "received " << raw.rows << "x" << raw.cols << std::endl;
    cv::Mat scaled;
    cv::pyrDown(raw, scaled);
    cv::imshow("1", scaled);
    return true;
}

int main()
{
    auto& lib = GXLibrary::instance();
    GXCamera cam(lib, callback);
    cv::namedWindow("1", 0);
    cam.run();
    while (cv::waitKey(0) != 'q') {}
    std::cout << "done" << std::endl;
    return 0;
}
