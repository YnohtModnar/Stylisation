#include "etf.hpp"

Point2f edgeTangentFlow(const Mat& isophote, const Mat& gHat, const int oRow,
                        const int oColumn, const int radius)
{
    int cRow, cColumn;
    float oGradMag, cGradMag, wMagnitude, wDirectional, vectorNormalizingTerm;
    Point2f oTan(isophote.at<float>(oRow, oColumn * 2 + 0),
                 isophote.at<float>(oRow, oColumn * 2 + 1));
    Point2f etf(0.0, 0.0);
    Point2f cTan(0.0,0.0);

    oGradMag = gHat.at<float>(oRow, oColumn);
    cGradMag = 0.0;
    vectorNormalizingTerm = 0.0;

    //for a given pixel we compute the etf inside a given radius
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            cRow = oRow + y;
            cColumn = oColumn + x;
            // if we are not in the given radius, no need for computation
            if (cRow > -1 && cRow < gHat.rows && cColumn > -1 &&
                cColumn < gHat.cols)
            {
                // if we respect the radius (pythagore) a²+b² = c²
                // spacial weight function, 0 if not inside the circle
                if ((x*x+y*y) <= radius*radius) {
                    cGradMag = gHat.at<float>(cRow, cColumn);
                    cTan = Point2f(isophote.at<float>(cRow, cColumn * 2 + 0),
                                   isophote.at<float>(cRow, cColumn * 2 + 1));
                    /* The magnitude weight function.
                    The result range between 0 and 1.
                    Bigger weight are given to pixel whose
                    gradient magnitude are higher than whose of origin pixel. */
                    wMagnitude = (cGradMag - oGradMag + 1.0) / 2.0;
                    /* compute directional weight, a bigger weight is given if
                    * vector as the same orientation */
                    wDirectional = oTan.dot(cTan);

                    etf += cTan * wMagnitude * wDirectional;
                    vectorNormalizingTerm += wMagnitude * wDirectional;
                }
            }
        }
    }
    // normalize etf
    etf = (vectorNormalizingTerm != 0.0)? etf/vectorNormalizingTerm : Point2f(0.0,0.0);

    return etf;
}

Mat computeETF(const Mat& iVectMap, const Mat& gHat, const int radius,
               int iteration)
{
    //temp will contains the value between two iterations
    Mat temp(gHat.rows, gHat.cols, CV_32FC2);
    Point2f etfPoint;
    // we use etf in our computation. Here etf is the isophote (the edges)
    Mat etf = iVectMap.clone();
    float vx, vy, mag;

    // we get a smoother etf after each iteration
    while (iteration--) {
        #pragma omp parallel for
        for (int y = 0; y < gHat.rows; y++) {
            for (int x = 0; x < gHat.cols; x++) {
                // compute the etf for a single pixel
                etfPoint = edgeTangentFlow(etf, gHat, y, x, radius);
                temp.at<float>(y, x * 2 + 0) = etfPoint.x;
                temp.at<float>(y, x * 2 + 1) = etfPoint.y;
            }
        }
        //normalize etf
        for (int y = 0; y < gHat.rows; y++) {
            for (int x = 0; x < gHat.cols; x++) {
                vx = temp.at<float>(y, x*2 + 0);
                vy = temp.at<float>(y, x*2 + 1);
                /* compute the magnitude to normalize. A 10e-8 is added to
                * avoid div by zero */
                mag = sqrt(vx*vx + vy*vy) + 1.0e-8;

                etf.at<float>(y, x * 2 + 0) = vx / mag;
                etf.at<float>(y, x * 2 + 1) = vy / mag;
            }
        }
    }

    return etf;
}
