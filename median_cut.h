/* Copyright (c) 2012 the authors listed at the following URL, and/or
the authors of referenced articles or incorporated external code:
http://en.literateprograms.org/Median_cut_algorithm_(C_Plus_Plus)?action=history&offset=20080309133934

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
luded in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Retrieved from: http://en.literateprograms.org/Median_cut_algorithm_(C_Plus_Plus)?oldid=12754
*/

#ifndef MEDIAN_CUT_H_
#define MEDIAN_CUT_H_
#include <list>
const int NUM_DIMENSIONS = 3;

#pragma pack(push)
#pragma pack(1)
struct Point
{
    unsigned char x[NUM_DIMENSIONS];
};
#pragma pack(pop)

class Block
{
    Point minCorner, maxCorner;
    Point* points;
    int pointsLength;
public:
    Block(Point* points, int pointsLength);
    Point * getPoints();
    int numPoints() const;
    int longestSideIndex() const;
    int longestSideLength() const;
    bool operator<(const Block& rhs) const;
    void shrink();
};
template <int index>
class CoordinatePointComparator
{
public:
    bool operator()(Point left, Point right)
    {
        return left.x[index] < right.x[index];
    }
};
std::list<Point> medianCut(Point* image, int numPoints, unsigned int desiredSize);
#endif /* #ifndef MEDIAN_CUT_H_ */
