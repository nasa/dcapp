#include <string>
#include <vector>
#include <cmath>
#include <cstring>
#include "RenderLib/RenderLib.hh"
#include "constants.hh"
#include "values.hh"
#include "commonutils.hh"
#include "adi.hh"

static const double BLACK[3] = { 0.0, 0.0, 0.0 };
static const double YELLOW[3] = { 1.0, 1.0, 0.0 };

dcADI::dcADI(dcParent *myparent) : dcGeometric(myparent), bkgdID(0x0), ballID(0x0), outerradius(0x0), ballradius(0x0), chevronW(0x0), chevronH(0x0)
{
    roll = getConstantFromDecimal(0);
    pitch = getConstantFromDecimal(0);
    yaw = getConstantFromDecimal(0);
    rollRate = getConstantFromDecimal(0);
    pitchRate = getConstantFromDecimal(0);
    yawRate = getConstantFromDecimal(0);
    rollError = getConstantFromDecimal(0);
    pitchError = getConstantFromDecimal(0);
    yawError = getConstantFromDecimal(0);
    rateMax = getConstantFromDecimal(0);
    scale = getConstantFromDecimal(1);
    rateMaxDefined = false;
    needleColor.set(YELLOW[0], YELLOW[1], YELLOW[2]);
    rateIndicatorColor.set(BLACK[0], YELLOW[1], YELLOW[2]);
    hideNeedlesFlag = false;
    hideRateIndicatorsFlag = true;
    sphereTriangles = BuildSphere();
}

void dcADI::setBackgroundTexture(const std::string &filename)
{
    if (!filename.empty()) bkgdID = tdLoadTexture(filename);
}

void dcADI::setBallTexture(const std::string &filename)
{
    if (!filename.empty()) ballID = tdLoadTexture(filename);
}

void dcADI::setRPY(const std::string &inroll, const std::string &inpitch, const std::string &inyaw)
{

    if (!inroll.empty()) roll = getValue(inroll);
    if (!inpitch.empty()) pitch = getValue(inpitch);
    if (!inyaw.empty()) yaw = getValue(inyaw);

}

void dcADI::setRPYerrors(const std::string &re, const std::string &pe, const std::string &ye)
{
    if (!re.empty()) rollError = getValue(re);
    if (!pe.empty()) pitchError = getValue(pe);
    if (!ye.empty()) yawError = getValue(ye);
}

void dcADI::setRPYRates(const std::string &inrollRate, const std::string &inpitchRate, const std::string &inyawRate, const std::string &inrateMax)
{
  
    if (!inrollRate.empty()) rollRate = getValue(inrollRate);
    if (!inpitchRate.empty()) pitchRate = getValue(inpitchRate);
    if (!inyawRate.empty()) yawRate = getValue(inyawRate);
    if (!inrateMax.empty()) {
      rateMax = getValue(inrateMax);
      rateMaxDefined = true;
    }
    else rateMaxDefined = false;
}

void dcADI::setNeedleColor(const std::string &nc)
{
    if (!nc.empty())
    {
        needleColor.set(nc);
    }
}

void dcADI::setRateIndicatorColor(const std::string &ric)
{
    if (!ric.empty())
    {
        rateIndicatorColor.set(ric);
    }
}


void dcADI::hideNeedles(const std::string &hn)
{
    if (!hn.empty()) hideNeedlesFlag = getValue(hn)->getBoolean();
    else hideNeedlesFlag = false;
}

void dcADI::hideRateIndicators(const std::string &hrn)
{
    if (!hrn.empty()) hideRateIndicatorsFlag = getValue(hrn)->getBoolean();
    else hideRateIndicatorsFlag = true;
}
  

void dcADI::setRadius(const std::string &outer, const std::string &ball)
{
    if (!outer.empty()) outerradius = getValue(outer);
    if (!ball.empty()) ballradius = getValue(ball);
}

void dcADI::setScale(const std::string &sc)
{
    if (!sc.empty()) scale = getValue(sc);
}

void dcADI::setChevron(const std::string &widthspec, const std::string &heightspec)
{
    if (!widthspec.empty()) chevronW = getValue(widthspec);
    if (!heightspec.empty()) chevronH = getValue(heightspec);
}

void dcADI::draw(void)
{

    computeGeometry();

    double outerrad, ballrad, chevw, chevh;

    if (outerradius) outerrad = outerradius->getDecimal();
    else outerrad = 0.5 * (fminf(w->getDecimal(), h->getDecimal()));

    if (ballradius) ballrad = ballradius->getDecimal();
    else ballrad = 0.9 * outerrad;

    if (chevronW) chevw = chevronW->getDecimal();
    else chevw = 0.2 * outerrad;

    if (chevronH) chevh = chevronH->getDecimal();
    else chevh = 0.2 * outerrad;

    // Draw the ball
    stencil_begin();        // enable stencil, clear existing buffer
    stencil_init_dest();    // setup stencil test to write 1's into destination area
        circle_fill(center, middle, ballrad, 360, 80, 1, 1, 1, 1);
    stencil_init_proj();    // set stencil to only keep fragments with reference != 0
        draw_textured_sphere(center, middle, sphereTriangles, ballrad * scale->getDecimal(), ballID, roll->getDecimal(), pitch->getDecimal(), yaw->getDecimal());
    stencil_end();

    // Draw the surrounding area (i.e. background)
    translate_start(left, bottom);
    draw_image(bkgdID, width, height);
    translate_end();


    // Draw the items on top: roll bug, cross-hairs, needles
    translate_start(center, middle);
        draw_top_rate_indicator(rollRate->getDecimal(), chevw, chevh, 0.8 * outerrad, rateMax->getDecimal());
	draw_bottom_rate_indicator(yawRate->getDecimal(), chevw, chevh, 0.8 * outerrad, rateMax->getDecimal());
	rotate_start(-90);
	draw_side_rate_indicator(pitchRate->getDecimal(), chevw, chevh, 0.8 * outerrad, rateMax->getDecimal());
	rotate_end();
        draw_roll_bug(roll->getDecimal(), chevw, chevh, 0.8 * outerrad);
        draw_cross_hairs(outerrad);
        draw_needles(outerrad, rollError->getDecimal(), pitchError->getDecimal(), yawError->getDecimal());
    translate_end();
}


void dcADI::draw_top_rate_indicator(double rate, double width, double height, double radius, double ratemax)
{

  if (hideRateIndicatorsFlag) return;
    //std::vector<float> pointsL = { 0, (float)radius, (float)width/2.0f, (float)(radius-height), -((float)width/2.0f), (float)(radius-height) };

  // float vertOffset = 65.0;

  //radius += vertOffset;
  radius = radius * 1.4;

  float maxValue = radius * .5; //Fix for green triangle going past bounds
  
  std::vector<float> pointsL = { 0, (float)(radius - height), -(float)width/2.0f, (float)(radius), ((float)width/2.0f), (float)(radius) };

  // scale rate to range [-120, 120]
  //rate = rate * 24.0;

  if (!rateMaxDefined) ratemax = 5.0;
  
  rate = rate * (maxValue / ratemax);
  if (rate > maxValue) rate = maxValue;
  else if (rate < -1 * maxValue) rate = -1 * maxValue;
  translate_start(rate, 0.0);
  draw_filled_triangles(pointsL, rateIndicatorColor.R->getDecimal(), rateIndicatorColor.G->getDecimal(), rateIndicatorColor.B->getDecimal(), 1);
        draw_line(pointsL, 1, BLACK[0], BLACK[1], BLACK[2], 1, 0xFFFF, 1);
	translate_end();
}

void dcADI::draw_bottom_rate_indicator(double rate, double width, double height, double radius, double ratemax)
{

  if (hideRateIndicatorsFlag) return;
    //std::vector<float> pointsL = { 0, (float)radius, (float)width/2.0f, (float)(radius-height), -((float)width/2.0f), (float)(radius-height) };

  // float vertOffset = 65.0;
  
  radius = radius * 1.4;

  float maxValue = radius * 0.5; // Fix for green triangle going past bounds
  
  std::vector<float> pointsL = { 0, -(float)(radius - height), -(float)width/2.0f, -(float)(radius), ((float)width/2.0f), -(float)(radius) };

  // scale rate to range [-120, 120]
  // rate = rate * 24.0;
  if (!rateMaxDefined) ratemax = 5.0;
  
  rate = rate * (maxValue / ratemax);
  if (rate > maxValue) rate = maxValue;
  else if (rate < -1 * maxValue) rate = -1 * maxValue;
  translate_start(rate, 0.0);
        draw_filled_triangles(pointsL, rateIndicatorColor.R->getDecimal(), rateIndicatorColor.G->getDecimal(), rateIndicatorColor.B->getDecimal(), 1);
        draw_line(pointsL, 1, BLACK[0], BLACK[1], BLACK[2], 1, 0xFFFF, 1);
	translate_end();
}

void dcADI::draw_side_rate_indicator(double rate, double width, double height, double radius, double ratemax)
{

    if (hideRateIndicatorsFlag) return;
    //std::vector<float> pointsL = { 0, (float)radius, (float)width/2.0f, (float)(radius-height), -((float)width/2.0f), (float)(radius-height) };

  //float vertOffset = 65.0;
  radius = radius * 1.4;

  float maxValue = radius * 0.5; //Fix for green triangles going past bounds
  std::vector<float> pointsL = { 0, (float)(radius - height), -(float)width/2.0f, (float)(radius), ((float)width/2.0f), (float)(radius) };


  if (!rateMaxDefined) ratemax = 5.0;
  
  // scale rate to range [-120, 120]
  // rate = rate * 24.0;
  rate = rate * (maxValue / ratemax);
  
  if (rate > maxValue) rate = maxValue;
  else if (rate < -1 * maxValue) rate = -1 * maxValue;
  translate_start(-1 * rate, 0.0);
        draw_filled_triangles(pointsL, rateIndicatorColor.R->getDecimal(), rateIndicatorColor.G->getDecimal(), rateIndicatorColor.B->getDecimal(), 1);
        draw_line(pointsL, 1, BLACK[0], BLACK[1], BLACK[2], 1, 0xFFFF, 1);
	translate_end();
}


void dcADI::draw_roll_bug(double roll, double width, double height, double radius)
{
    std::vector<float> pointsL = { 0, (float)radius, (float)width/2.0f, (float)(radius-height), -((float)width/2.0f), (float)(radius-height) };

    rotate_start(roll);
        draw_filled_triangles(pointsL, YELLOW[0], YELLOW[1], YELLOW[2], 1);
        draw_line(pointsL, 1, BLACK[0], BLACK[1], BLACK[2], 1, 0xFFFF, 1);
    rotate_end();
}

void dcADI::draw_cross_hairs(double radius)
{
    const float length = 0.21 * radius;
    const float halfwidth = 0.017 * radius;
    std::vector<float> polygonH, polygonV, crosshair;

    // horizontal rectangle
    addPoint(polygonH, -length,  halfwidth);
    addPoint(polygonH,  length,  halfwidth);
    addPoint(polygonH,  length, -halfwidth);
    addPoint(polygonH, -length, -halfwidth);
    //draw_quad(polygonH, YELLOW[0], YELLOW[1], YELLOW[2], 1);
    draw_quad(polygonH, needleColor.R->getDecimal(), needleColor.G->getDecimal(), needleColor.B->getDecimal(), 1);

    // vertical rectangle
    addPoint(polygonV, -halfwidth,  length);
    addPoint(polygonV,  halfwidth,  length);
    addPoint(polygonV,  halfwidth, -length);
    addPoint(polygonV, -halfwidth, -length);
    // draw_quad(polygonV, YELLOW[0], YELLOW[1], YELLOW[2], 1);
    draw_quad(polygonV, needleColor.R->getDecimal(), needleColor.G->getDecimal(), needleColor.B->getDecimal(), 1);

    // outline
    addPoint(crosshair, -length, halfwidth);
    addPoint(crosshair, -halfwidth, halfwidth);
    addPoint(crosshair, -halfwidth, length);
    addPoint(crosshair, halfwidth, length);
    addPoint(crosshair, halfwidth, halfwidth);
    addPoint(crosshair, length, halfwidth);
    addPoint(crosshair, length, -halfwidth);
    addPoint(crosshair, halfwidth, -halfwidth);
    addPoint(crosshair, halfwidth, -length);
    addPoint(crosshair, -halfwidth, -length);
    addPoint(crosshair, -halfwidth, -halfwidth);
    addPoint(crosshair, -length, -halfwidth);
    addPoint(crosshair, -length, halfwidth);
    draw_line(crosshair, 1, BLACK[0], BLACK[1], BLACK[2], 1, 0xFFFF, 1);
}

void dcADI::draw_needles(double radius, double roll_err, double pitch_err, double yaw_err)
{

    if (hideNeedlesFlag) return;
    // ERROR NEEDLE SIZE SHIFT
    radius = radius * 0.85;
    const double length = 0.65 * radius;
    //const double length = 0.01 * radius;
    const double halfwidth = 0.017 * radius;
    double delta, needle_edge;
    std::vector<float> pntsL;

    delta = get_error_info(yaw_err, radius);
    needle_edge = radius*(cos(asin(delta/radius)));

    addPoint(pntsL, delta-halfwidth, -needle_edge);
    addPoint(pntsL, delta-halfwidth, -length);
    addPoint(pntsL, delta+halfwidth, -length);
    addPoint(pntsL, delta+halfwidth, -needle_edge);
    //draw_quad(pntsL, YELLOW[0], YELLOW[1], YELLOW[2], 1);
    if (!hideNeedlesFlag) draw_quad(pntsL, needleColor.R->getDecimal(), needleColor.G->getDecimal(), needleColor.B->getDecimal(), 1);

    pntsL.clear();
    addPoint(pntsL, delta-halfwidth, -needle_edge );
    addPoint(pntsL, delta-halfwidth, -length );
    addPoint(pntsL, delta+halfwidth, -length );
    addPoint(pntsL, delta+halfwidth, -needle_edge );
    draw_line(pntsL, 1, BLACK[0], BLACK[1], BLACK[2], 1, 0xFFFF, 1);

    delta = get_error_info(roll_err, radius);
    needle_edge = radius*(cos(asin(delta/radius)));

    pntsL.clear();
    addPoint(pntsL, delta-halfwidth, needle_edge);
    addPoint(pntsL, delta-halfwidth, length);
    addPoint(pntsL, delta+halfwidth, length);
    addPoint(pntsL, delta+halfwidth, needle_edge);
    //draw_quad(pntsL, YELLOW[0], YELLOW[1], YELLOW[2], 1);
    if (!hideNeedlesFlag) draw_quad(pntsL, needleColor.R->getDecimal(), needleColor.G->getDecimal(), needleColor.B->getDecimal(), 1);

    pntsL.clear();
    addPoint(pntsL, delta-halfwidth, needle_edge );
    addPoint(pntsL, delta-halfwidth, length );
    addPoint(pntsL, delta+halfwidth, length );
    addPoint(pntsL, delta+halfwidth, needle_edge );
    draw_line(pntsL, 1, BLACK[0], BLACK[1], BLACK[2], 1, 0xFFFF, 1);

    delta = get_error_info(pitch_err, radius);
    needle_edge = radius*(cos(asin(delta/radius)));

    pntsL.clear();
    addPoint(pntsL, needle_edge, delta-halfwidth);
    addPoint(pntsL, length, delta-halfwidth);
    addPoint(pntsL, length, delta+halfwidth);
    addPoint(pntsL, needle_edge, delta+halfwidth);
    //    draw_quad(pntsL, YELLOW[0], YELLOW[1], YELLOW[2], 1);
    if (!hideNeedlesFlag) draw_quad(pntsL, needleColor.R->getDecimal(), needleColor.G->getDecimal(), needleColor.B->getDecimal(), 1);

    pntsL.clear();
    addPoint(pntsL, needle_edge, delta-halfwidth );
    addPoint(pntsL, length, delta-halfwidth );
    addPoint(pntsL, length, delta+halfwidth );
    addPoint(pntsL, needle_edge, delta+halfwidth );
    draw_line(pntsL, 1, BLACK[0], BLACK[1], BLACK[2], 1, 0xFFFF, 1);
}

double dcADI::get_error_info(double value, double outer_rad)
{
    if (value < -1.0) return (0.4 * outer_rad);
    else if (value > 1.0) return (-0.4 * outer_rad);
    else return (-0.4 * outer_rad * value);
}

/* Build a sphere with triangles */

void dcADI::BuildTriangles(std::vector<float> &listA, const std::vector<LocalPoint> &list1A, const std::vector<LocalPoint> &list2A)
{
// list1A and list2A are vectors of 3D points forming a circle at two different latitudes on a sphere.
// We will create 2 triangles between 4 points (2 from each vector);

/*
        P1a    P2a  P3a   P4   P5a
                -------------------------
                |    /|    /|    /|    /|
                |   / |   / |   / |   / |
                |  /  |  /  |  /  |  /  |
                | /   | /   | /   | /   |
                |/    |/    |/    |/    |
                -------------------------
        P1b   P2b   P3b   P4b    P5b


P1a should be equal to P5a
P1b should be equal to P5b
*/

    auto itr1L = std::begin(list1A);
    auto itr2L = std::begin(list2A);
    auto end1L = std::end(list1A);

// get the next points in each vector
    auto itr3L = std::next(itr1L);
    auto itr4L = std::next(itr2L);

    while (itr3L != end1L)
    {
        {
            auto p1L = (*itr1L);
            auto p2L = (*itr2L);
            auto p3L = (*itr4L);

            addPoint(listA, p1L.m_X, p1L.m_Y, p1L.m_Z, p1L.m_U, p1L.m_V);
            addPoint(listA, p2L.m_X, p2L.m_Y, p2L.m_Z, p2L.m_U, p2L.m_V);
            addPoint(listA, p3L.m_X, p3L.m_Y, p3L.m_Z, p3L.m_U, p3L.m_V);
        }

        {
            auto p1L = (*itr1L);
            auto p2L = (*itr4L);
            auto p3L = (*itr3L);

            addPoint(listA, p1L.m_X, p1L.m_Y, p1L.m_Z, p1L.m_U, p1L.m_V);
            addPoint(listA, p2L.m_X, p2L.m_Y, p2L.m_Z, p2L.m_U, p2L.m_V);
            addPoint(listA, p3L.m_X, p3L.m_Y, p3L.m_Z, p3L.m_U, p3L.m_V);
        }

// get the next points in each vector
        itr3L = std::next(itr3L);
        itr4L = std::next(itr4L);

        itr1L = std::next(itr1L);
        itr2L = std::next(itr2L);
    }
}

std::vector<float> dcADI::BuildSphere(void)
{
    const int numLatL  = 24;
    const int numLongL = 24;
    float radiusL      = 1.0f;
    float deltalongL   = 2.0 * M_PI / static_cast<float>(numLongL);
    float deltalatL    = M_PI / static_cast<float>(numLatL-1);

    std::vector<std::vector<LocalPoint>> pointListL;

    // Build all points needed for sphere

    // This does not start at 0; it starts one latitude(row) up and ends one
    // latitude(row) down from 1.  The top and bottom are done as a fan at the
    // bottom of this loop.

    // The north pole and south pole are drawn as a triangle fan; So we only need the point in the middle and then use the
    // first and last row of points generated below.

    // this starts the list of points one step up from the south pole.
    // we are building a vector of vectors containing Local Point, so it is a 2D grid of 3D points
    // where each row (index into pointList) is a vector<LocalPoint> that contains numLongL points on a circle around a sphere at anglelatL latitude.
    // Because the inner loop goes to numLongL, anglelongL will wrap around from [0, 360] causing the first and last point to be the same. This is done
    // to make the generation of the triangle from the list of points easier; no special case will be required to go from the last point to the first point
    float anglelatL = deltalatL;

    for (size_t iL=0; iL<numLatL-2; iL++)
    {
        std::vector<LocalPoint> pointsL;

        // start at longitude 0
        float anglelongL = 0.0f;
        float radiuslatL = std::abs(sinf(anglelatL) * radiusL);
        float zL = cosf(anglelatL) * radiusL;

        // We want the first and last points to be the same
        for (size_t jL=0; jL<numLongL+1; jL++)
        {
            // storage for one 3D point on this circle at latitude anglelatL
            // C++ 17 this will become LocalPoint &pointL = pointsL.emplace_back(LocalPoint());
            pointsL.emplace_back(LocalPoint());
            LocalPoint &pointL = pointsL.back();

            // the sphere is built along the x axis, so m_X gets the Z value, and m_Y, m_Z is the point on the circle at this "height" of the sphere
            pointL.m_Y = (cosf(anglelongL) * radiuslatL);
            pointL.m_Z = (sinf(anglelongL) * radiuslatL);
            pointL.m_X = zL;

            // m_U and m_V go from [0,1].
            // m_V goes from top to bottom along the X axis
            // m_U goes around the circle, we divide anglelongL by 2PI to get [0,1]
            pointL.m_U = std::abs(anglelongL / (2.0 * M_PI));
            pointL.m_V = 1.0 - anglelatL / M_PI;

            pointsL.push_back(pointL);

            anglelongL += deltalongL;
        }

        pointListL.push_back(pointsL);

        anglelatL += deltalatL;
    }

    // trianglePoints is a list of points which form triangles around the sphere.  Every 3 points is a triangle.
    std::vector<float> trianglePoints;

    // For every two "rows" of points around the sphere; this is the 2D grid of 3D points built above
    // build a set of triangles which will fill there area between the points; this will be two triangles for each set of 4 points (2 from each row)
    for (size_t iL=0; iL<pointListL.size()-1; iL++)
        BuildTriangles(trianglePoints, pointListL[iL], pointListL[iL+1]);

    // Build Bottom EndCap
    auto itr1L = pointListL[0].begin();
    auto end1L = pointListL[0].end();
    auto itr3L = std::next(itr1L);

    while (itr3L != end1L)
    {
        trianglePoints.push_back(1.0);
        trianglePoints.push_back(0.0);
        trianglePoints.push_back(0.0);
        trianglePoints.push_back(0.5);
        trianglePoints.push_back(1.0);

        addPoint(trianglePoints, itr1L->m_X, itr1L->m_Y, itr1L->m_Z, itr1L->m_U, itr1L->m_V);
        addPoint(trianglePoints, itr3L->m_X, itr3L->m_Y, itr3L->m_Z, itr3L->m_U, itr3L->m_V);

        itr1L = std::next(itr1L);
        itr3L = std::next(itr1L);
    }

    // Build Top EndCap
    itr1L = pointListL[pointListL.size()-1].begin();
    end1L = pointListL[pointListL.size()-1].end();
    itr3L = std::next(itr1L);

    while (itr3L != end1L)
    {
        trianglePoints.push_back(-1.0);
        trianglePoints.push_back(0.0);
        trianglePoints.push_back(0.0);
        trianglePoints.push_back(0.5);
        trianglePoints.push_back(0.0);

        addPoint(trianglePoints, itr3L->m_X, itr3L->m_Y, itr3L->m_Z, itr3L->m_U, itr3L->m_V);
        addPoint(trianglePoints, itr1L->m_X, itr1L->m_Y, itr1L->m_Z, itr1L->m_U, itr1L->m_V);

        itr1L = std::next(itr1L);
        itr3L = std::next(itr1L);
    }

    return trianglePoints;
}
