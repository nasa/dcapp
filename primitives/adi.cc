#include <cmath>
#include <cstring>
#include "loadUtils.hh"
#include "opengl_draw.hh"
#include "varlist.hh"
#include "adi.hh"

static const float BLACK[3] = {0.0, 0.0, 0.0};
static const float YELLOW[3] = {1.0, 1.0, 0.0};

dcADI::dcADI(dcParent *myparent) : dcGeometric(myparent), bkgdID(-1), ballID(-1), outerradius(0x0), ballradius(0x0), chevronW(0x0), chevronH(0x0)
{
    roll = dcLoadConstant(0.0f);
    pitch = dcLoadConstant(0.0f);
    yaw = dcLoadConstant(0.0f);
    rollError = dcLoadConstant(0.0f);
    pitchError = dcLoadConstant(0.0f);
    yawError = dcLoadConstant(0.0f);
	
	
	sphereTriangles = BuildSphere();
}

void dcADI::setBackgrountTexture(const char *filename)
{
    if (filename) bkgdID = dcLoadTexture(filename);
}

void dcADI::setBallTexture(const char *filename)
{
    if (filename) ballID = dcLoadTexture(filename);
}

void dcADI::setRPY(const char *inroll, const char *inpitch, const char *inyaw)
{
    if (inroll) roll = getFloatPointer(inroll);
    if (inpitch) pitch = getFloatPointer(inpitch);
    if (inyaw) yaw = getFloatPointer(inyaw);
}

void dcADI::setRPYerrors(const char *re, const char *pe, const char *ye)
{
    if (re) rollError = getFloatPointer(re);
    if (pe) pitchError = getFloatPointer(pe);
    if (ye) yawError = getFloatPointer(ye);
}

void dcADI::setRadius(const char *outer, const char *ball)
{
    if (outer) outerradius = getFloatPointer(outer);
    if (ball) ballradius = getFloatPointer(ball);
}

void dcADI::setChevron(const char *width, const char *height)
{
    if (width) chevronW = getFloatPointer(width);
    if (height) chevronH = getFloatPointer(height);
}

void dcADI::draw(void)
{
    computeGeometry();

    float outerrad, ballrad, chevw, chevh;

    if (outerradius) outerrad = *outerradius;
    else outerrad = 0.5 * (fminf(*w, *h));

    if (ballradius) ballrad = *ballradius;
    else ballrad = 0.9 * outerrad;

    if (chevronW) chevw = *chevronW;
    else chevw = 0.2 * outerrad;

    if (chevronH) chevh = *chevronH;
    else chevh = 0.2 * outerrad;


    // Draw the ball
//    draw_textured_sphere(center, middle, ballrad, ballID, *roll, *pitch, *yaw);
    draw_textured_sphere( center, middle, sphereTriangles, ballrad, ballID, *roll, *pitch, *yaw );

    // Draw the surrounding area (i.e. background)
    translate_start(left, bottom);
    draw_image(bkgdID, width, height);
    translate_end();

    // Draw the items on top: roll bug, cross-hairs, needles
    translate_start(center, middle);
        draw_roll_bug(*roll, chevw, chevh, 0.8 * outerrad);
        draw_cross_hairs(outerrad);
        draw_needles(outerrad, *rollError, *pitchError, *yawError);
    translate_end();
}

void dcADI::draw_roll_bug(float roll, float width, float height, float radius)
{
	std::vector< float > pointsL = { 0, radius, width/2.0f, radius-height, -(width/2.0f), radius-height };

    rotate_start(roll);
		draw_filled_triangles( pointsL, YELLOW[0], YELLOW[1], YELLOW[2], 1 );
		draw_line( pointsL, 1, BLACK[0], BLACK[1], BLACK[2], 1 );
    rotate_end();
}

void dcADI::draw_cross_hairs(float radius)
{
	const float length = 0.21 * radius;
	const float halfwidth = 0.017 * radius;
	std::vector< float > crosshair;
	std::vector< float > polygonL;
	
    addPoint( crosshair, -length, halfwidth );
    addPoint( crosshair, -halfwidth, halfwidth );
    addPoint( crosshair, -halfwidth, length );
    addPoint( crosshair, halfwidth, length );
    addPoint( crosshair, halfwidth, halfwidth );
    addPoint( crosshair, length, halfwidth );
    addPoint( crosshair, length, -halfwidth );
    addPoint( crosshair, halfwidth, -halfwidth );
    addPoint( crosshair, halfwidth, -length );
    addPoint( crosshair, -halfwidth, -length );
    addPoint( crosshair, -halfwidth, -halfwidth );
    addPoint( crosshair, -length, -halfwidth );
	
	// tri 1
	addPoint( polygonL, -length,  halfwidth );
	addPoint( polygonL,  length,  halfwidth );
	addPoint( polygonL,  length, -halfwidth );
	addPoint( polygonL, -length, -halfwidth );
	
	draw_filled_triangles( polygonL, YELLOW[0], YELLOW[1], YELLOW[2], 1 );

	// add starting point for line to end to close the loop
	crosshair.push_back( -length );
	crosshair.push_back( halfwidth );
	draw_line( crosshair, 1, BLACK[0], BLACK[1], BLACK[2], 1 );
}

void dcADI::draw_needles(float radius, float roll_err, float pitch_err, float yaw_err)
{
    const float length = 0.29 * radius;
	const float halfwidth = 0.017 * radius;
	float delta, needle_edge;
	std::vector<float>	pntsL;

    delta = get_error_info(yaw_err, radius);
    needle_edge = radius*(cos(asin(delta/radius)));
	
	addPoint( pntsL, delta-halfwidth, -needle_edge);
	addPoint( pntsL, delta-halfwidth, -length);
	addPoint( pntsL, delta+halfwidth, -length);
	addPoint( pntsL, delta+halfwidth, -needle_edge);
	
	draw_quad( pntsL, YELLOW[0], YELLOW[1], YELLOW[2], 1 );
	
	pntsL.clear();
	addPoint( pntsL, delta-halfwidth, -needle_edge );
	addPoint( pntsL, delta-halfwidth, -length );
	addPoint( pntsL, delta+halfwidth, -length );
	addPoint( pntsL, delta+halfwidth, -needle_edge );

	draw_line( pntsL, 1, BLACK[0], BLACK[1], BLACK[2], 1 );

    delta = get_error_info(roll_err, radius);
    needle_edge = radius*(cos(asin(delta/radius)));

	pntsL.clear();
	addPoint( pntsL, delta-halfwidth, needle_edge);
	addPoint( pntsL, delta-halfwidth, length);
	addPoint( pntsL, delta+halfwidth, length);
	addPoint( pntsL, delta+halfwidth, needle_edge);
	draw_quad( pntsL, YELLOW[0], YELLOW[1], YELLOW[2], 1 );

	pntsL.clear();
	addPoint( pntsL, delta-halfwidth, needle_edge );
	addPoint( pntsL, delta-halfwidth, length );
	addPoint( pntsL, delta+halfwidth, length );
	addPoint( pntsL, delta+halfwidth, needle_edge );
	draw_line( pntsL, 1, BLACK[0], BLACK[1], BLACK[2], 1 );

    delta = get_error_info(pitch_err, radius);
    needle_edge = radius*(cos(asin(delta/radius)));

	pntsL.clear();
	addPoint( pntsL, needle_edge, delta-halfwidth);
	addPoint( pntsL, length, delta-halfwidth);
	addPoint( pntsL, length, delta+halfwidth);
	addPoint( pntsL, needle_edge, delta+halfwidth);
	draw_quad( pntsL, YELLOW[0], YELLOW[1], YELLOW[2], 1 );
	
	pntsL.clear();
	addPoint( pntsL, needle_edge, delta-halfwidth );
	addPoint( pntsL, length, delta-halfwidth );
	addPoint( pntsL, length, delta+halfwidth );
	addPoint( pntsL, needle_edge, delta+halfwidth );
	draw_line( pntsL, 1, BLACK[0], BLACK[1], BLACK[2], 1 );
}

float dcADI::get_error_info(float value, float outer_rad)
{
    if (value < -1.0) return (0.4 * outer_rad);
    else if (value > 1.0) return (-0.4 * outer_rad);
    else return (-0.4 * outer_rad * value);
}

/* Build a sphere with triangles */

void dcADI::BuildTriangles( std::vector<float> &listA, const std::vector<LocalPoint> &list1A, const std::vector<LocalPoint> &list2A  )
{
	auto itr1L = std::begin( list1A );
	auto itr2L = std::begin( list2A );
	auto end1L = std::end( list1A );
	auto end2L = std::end( list2A );
	
	size_t indexL = 0;
	while( itr1L != end1L )
	{
		auto itr3L = std::next( itr1L );
		auto itr4L = std::next( itr2L );
		
		if( itr3L == end1L )
			itr3L = std::begin( list1A );
		
		if( itr4L == end2L )
			itr4L = std::begin( list2A );
		
		{
			auto p1L = (*itr1L);
			auto p2L = (*itr2L);
			auto p3L = (*itr4L);
			
			if( indexL == (list1A.size() - 1) )
				p3L.m_U += 1.0;
			
			addPoint( listA, p1L.m_X, p1L.m_Y, p1L.m_Z, p1L.m_U, p1L.m_V );
			addPoint( listA, p2L.m_X, p2L.m_Y, p2L.m_Z, p2L.m_U, p2L.m_V );
			addPoint( listA, p3L.m_X, p3L.m_Y, p3L.m_Z, p3L.m_U, p3L.m_V );
		}
		
		{
			auto p1L = (*itr1L);
			auto p2L = (*itr4L);
			auto p3L = (*itr3L);
			if( indexL == (list1A.size() - 1) )
			{
				p2L.m_U += 1.0;
				p3L.m_U += 1.0;
			}
			
			addPoint( listA, p1L.m_X, p1L.m_Y, p1L.m_Z, p1L.m_U, p1L.m_V );
			addPoint( listA, p2L.m_X, p2L.m_Y, p2L.m_Z, p2L.m_U, p2L.m_V );
			addPoint( listA, p3L.m_X, p3L.m_Y, p3L.m_Z, p3L.m_U, p3L.m_V );
		}
		
		itr1L = std::next( itr1L );
		itr2L = std::next( itr2L );
	}
}

std::vector<float> dcADI::BuildSphere( void )
{
	const int numLatL	= 10;
	const int numLongL	= 10;
	float radiusL		= 1.0;
	float deltalongL	= 2.0 * M_PI / static_cast<float>(numLongL);
	float deltalatL		= M_PI / static_cast<float>(numLatL);
	
	std::vector< std::vector< LocalPoint > >	pointListL;

	//
	// Build all points needed for sphere
	
	// This does not start at 0; it starts one latitude(row) up and
	// ends one latitude(row) down from 1.  The top and bottom are done
	// as a fan at the bottom of this loop.
	float anglelatL = deltalatL;
	
	for( size_t iL=0; iL<numLatL-1; iL++ )
	{
		std::vector< LocalPoint > pointsL;
		float anglelongL = 0.0f;
		float radiuslatL = std::abs(sinf(anglelatL) * radiusL);
		float zL = cosf( anglelatL ) * radiusL;

		for( size_t jL=0; jL<numLongL; jL++ )
		{
			// C++ 17 this will become LocalPoint &pointL = pointsL.emplace_back( LocalPoint() );
			pointsL.emplace_back( LocalPoint() );
			LocalPoint &pointL = pointsL.back();
			
			pointL.m_Y = (cosf( anglelongL ) * radiuslatL );
			pointL.m_Z = (sinf( anglelongL ) * radiuslatL );
			pointL.m_X = zL;
			
			pointL.m_U = std::abs( anglelongL / (2.0 * M_PI) ) * 2.0f;
			pointL.m_V = 1.0 - anglelatL / M_PI;
			
			pointsL.push_back( pointL );
			
			anglelongL += deltalongL;
		}
		
		pointListL.push_back( pointsL );
		
		anglelatL += deltalatL;
	}
	
	std::vector< float > trianglePoints;
	for( size_t iL=0; iL<numLatL-2; iL++ )
		BuildTriangles( trianglePoints, pointListL[iL], pointListL[iL+1] );

	//
	// Build Bottom EndCap
	
	auto itr1L = pointListL[0].begin();
	auto end1L = pointListL[0].end();
	
	while( itr1L != end1L )
	{
		auto itr3L = itr1L + 1;
		
		if( itr3L == end1L )
			itr3L = pointListL[0].begin();
		
		{
			trianglePoints.push_back( 0.5 );
			trianglePoints.push_back( 0.0 );
			trianglePoints.push_back( 0.0 );
			trianglePoints.push_back( 0.5 );
			trianglePoints.push_back( 1.0 );

			trianglePoints.push_back( itr1L->m_X );
			trianglePoints.push_back( itr1L->m_Y );
			trianglePoints.push_back( itr1L->m_Z );
			trianglePoints.push_back( itr1L->m_U );
			trianglePoints.push_back( itr1L->m_V );
			
			trianglePoints.push_back( itr3L->m_X );
			trianglePoints.push_back( itr3L->m_Y );
			trianglePoints.push_back( itr3L->m_Z );
			trianglePoints.push_back( itr3L->m_U );
			trianglePoints.push_back( itr3L->m_V );
		}
		
		++itr1L;
	}
	
	// Build Top EndCap
	itr1L = pointListL[numLatL-2].begin();
	end1L = pointListL[numLatL-2].end();
	
	while( itr1L != end1L )
	{
		auto itr3L = itr1L + 1;
		
		if( itr3L == end1L )
			itr3L = pointListL[numLatL-2].begin();
		
		{
			trianglePoints.push_back( -0.5 );
			trianglePoints.push_back( 0.0 );
			trianglePoints.push_back( 0.0 );
			trianglePoints.push_back( 0.5 );
			trianglePoints.push_back( 0.0 );
			
			trianglePoints.push_back( itr3L->m_X );
			trianglePoints.push_back( itr3L->m_Y );
			trianglePoints.push_back( itr3L->m_Z );
			trianglePoints.push_back( itr3L->m_U );
			trianglePoints.push_back( itr3L->m_V );
			
			trianglePoints.push_back( itr1L->m_X );
			trianglePoints.push_back( itr1L->m_Y );
			trianglePoints.push_back( itr1L->m_Z );
			trianglePoints.push_back( itr1L->m_U );
			trianglePoints.push_back( itr1L->m_V );
		}
		
		++itr1L;
	}
	
	return trianglePoints;
}
