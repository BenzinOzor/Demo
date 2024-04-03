#include <FZN/Tools/Math.h>
#include <FZN/Tools/Tools.h>

#include "Simplex.h"


bool Simplex::ContainsOrigin( sf::Vector2f& _vDirection )
{
	// https://www.youtube.com/watch?v=Qupqu1xe7Io
	// https://dyn4j.org/2010/04/gjk-gilbert-johnson-keerthi/

	auto DoSimplexLine = [&]( sf::Vector2f& _vDirection )
	{
		const sf::Vector2f& vA = m_daPoints.back();		// A
		const sf::Vector2f vAO = -vA;					// [AO]
		const sf::Vector2f vAB = m_daPoints[ 0 ] - vA;	// [AB]

		// If [AB] and [AO] are going in the same direction, it means that the origin is between A and B
		// We'll then have to look for the farthest point in the direction of the vector perpendicular to AB going in the direction of the origin
		if( fzn::Math::VectorsSameDirection( vAB, vAO ) )
		{
			// [AB] x [AO] x [AB]
			_vDirection = fzn::Math::VectorsGetPerpendicular( vAB, vAO );
		}
		else
			_vDirection = vAO;	// If not, we simply search in the direction of the origin ( [AO] )
	};

	auto DoSimplexTriangle = [&]( sf::Vector2f& _vDirection ) -> bool
	{
		// Is the origin in the current triangle
		if( fzn::Tools::CollisionOBBPoint( m_daPoints, { 0.f, 0.f } ) )
			return true;

		const sf::Vector2f& vA = m_daPoints.back();
		const sf::Vector2f& vB = m_daPoints[ 0 ];
		const sf::Vector2f& vC = m_daPoints[ 1 ];
		const sf::Vector2f vAO = -vA;
		const sf::Vector2f vAB = vB - vA;
		const sf::Vector2f vAC = vC - vA;

		// If the origin isn't in the triangle, we have to determine in which direction we will look for the origin.
		// We will have to test several regions:
		// RAB: The region perpendicular to [AB]
		// RAC: The region perpendicular to [AC]
		// RA: The region starting from point A, between RAB and RAC.

		// All the other regions have been eliminated before, that's why we are here, the origin can't be in these regions:
		// RBC: The region perpendicular to [BC], because we eliminated it by going in the opposite direction to get the current point A.
		// RB: The region starting from point B, between RBC and RAB.
		// RC: The region starting from point C, between RBC and RAC.

		// Now we compute the normal to the ABC plane. It will be used to compute [AB] and [AC] normals when we need them.
		const sf::Vector2f vPlaneNormal = fzn::Math::VectorCross2D( vAB, vAC );

		// We compute the first normal which will be the one perpendicular to [AB], called nAB.
		sf::Vector2f vNormal{ fzn::Math::VectorCross2D( vPlaneNormal, vAB ) };

		// nAB . [AO] && [AB] . [AO]
		// If [AO] and nAB are going in the same diretion and [AB] and [AO] are going in the same direction aswell, it means the origin is in RAB
		if( fzn::Math::VectorsSameDirection( vNormal, vAO ) && fzn::Math::VectorsSameDirection( vAB, vAO ) )
		{
			// We now have to remove point C from the simplex as it is the farthest from the origin and can't be of help anymore.
			RemovePoint( 1 );

			// We compute the next direction we want to use to find the origin, which is the vector perpendicular to [AB] in the direction of [AO]
			_vDirection = fzn::Math::VectorsGetPerpendicular( vAB, vAO );
			return false;
		}
		else
		{
			// We compute the second normal which will be the one perpendicular to [AC], called nAC.
			vNormal = fzn::Math::VectorCross2D( vPlaneNormal, vAC );

			// nAC . [AO] && [AC] . [AO]
			// If [AO] and nAC are going in the same diretion and [AC] and [AO] are going in the same direction aswell, it means the origin is in RAC
			if( fzn::Math::VectorsSameDirection( vNormal, vAO ) && fzn::Math::VectorsSameDirection( vAC, vAO ) )
			{
				// We now have to remove point B from the simplex as it is the farthest from the origin and can't be of help anymore.
				RemovePoint( 0 );

				// We compute the next direction we want to use to find the origin, which is the vector perpendicular to [AC] in the direction of [AO]
				_vDirection = fzn::Math::VectorsGetPerpendicular( vAC, vAO );
				return false;
			}
		}

		// If we get here, it means the origin was found nowhere and so, we're going to search it in the last region, RA.
		// The next direction is the vector going from point A to the origin, [AO].
		_vDirection = vAO;
		return false;
	};

	// If the simplex is only a point, it can't contain anything and we can't determine where to go.
	if( m_daPoints.size() < 2 )
		return false;

	// If the simplex is a line, we have to determine in which direction to go to create the triangle.
	if( m_daPoints.size() == 2 )
	{
		DoSimplexLine( _vDirection );
		return false;
	}
	else
		return DoSimplexTriangle( _vDirection );
}

void Simplex::AddPoint( const sf::Vector2f& _vPoint )
{
	m_daPoints.push_back( _vPoint );
}

void Simplex::RemovePoint( int _iPointIndex )
{
	if( _iPointIndex >= m_daPoints.size() )
		return;

	m_daPoints.erase( m_daPoints.begin() + _iPointIndex );
}
