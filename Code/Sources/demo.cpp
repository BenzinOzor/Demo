//------------------------------------------------------------------------
//Author : Philippe OFFERMANN
//Date : 29.01.15
//Description : Demonstration class
//------------------------------------------------------------------------
#include <array>

#include <Externals/ImGui/imgui.h>
#include <Externals/ImGui/imgui-SFML.h>

#include <FZN/Includes.h>
#include <FZN/Managers/WindowManager.h>
#include <FZN/Managers/DataManager.h>
#include <FZN/Managers/InputManager.h>
#include <FZN/Managers/AnimManager.h>
#include <FZN/Tools/Math.h>
#include <FZN/Audio/Sound.h>
#include <FZN/Audio/AudioObject.h>
#include <FZN/DataStructure/Map.h>
#include <FZN/Tools/Random.h>

#include "demo.h"
#include <iostream>
#include <Fmod/fmod.hpp>

const float SPLINE_TARGET_RADIUS = 10.f;

#pragma region JumpTest
void JumpTest::Update()
{
	sf::Vector2f vCurrentPosition = getPosition();
	sf::Vector2f vDirection = sf::Vector2f( 0.f, 0.f );
	sf::Vector2f vLat = sf::Vector2f( 0.f, 0.f );

	if( vCurrentPosition.y == 700.f && g_pFZN_InputMgr->IsActionPressed( "Jump" ) )
	{
		m_fInitialDirection = sf::Vector2f( m_fCurrentDirection.x, -500.f );
		m_fCurrentDirection = m_fInitialDirection;
	}
	if( g_pFZN_InputMgr->IsActionPressed( "Right" ) || g_pFZN_InputMgr->IsActionDown( "Right" ) )
		vLat = sf::Vector2f( 200.f, 0.f );
	else if( g_pFZN_InputMgr->IsActionPressed( "Left" ) || g_pFZN_InputMgr->IsActionDown( "Left" ) )
		vLat = sf::Vector2f( -200.f, 0.f ); 

	vDirection = m_fCurrentDirection + m_fGravity * FrameTime;
	vDirection += vLat * 2.f * FrameTime;

	vDirection.x = fzn::Math::Clamp( vDirection.x, -200.f, 200.f );

	vCurrentPosition += vDirection * FrameTime;
	m_fCurrentDirection = vDirection;

	if( vCurrentPosition.y > 700.f )
		vCurrentPosition.y = 700.f;

	setPosition( vCurrentPosition );

	sf::CircleShape* circle = new sf::CircleShape( 2.f );
	circle->setFillColor( sf::Color::Red );
	circle->setPosition( vCurrentPosition );
	m_fPath.push_back( circle );


	for( int i = 0 ; i < m_fPath.size() ; ++i )
		g_pFZN_WindowMgr->Draw( *m_fPath[i] );
	g_pFZN_WindowMgr->Draw( *( sf::CircleShape* )this );
}
#pragma endregion

#pragma region PhyTest
void PhyTest::Update()
{
	if( m_bUseHorizontalMaxValues )
		m_vVelocity.x = fzn::Math::Clamp( m_vVelocity.x, -m_vMaxValues.x, m_vMaxValues.x );
	if( m_bUseVerticalMaxValues )
		m_vVelocity.y = fzn::Math::Clamp( m_vVelocity.y, -m_vMaxValues.y, m_vMaxValues.y );

	m_vDirection += m_vGravity;

	m_vVelocity = fzn::Math::VectorTruncated( m_vVelocity + m_vDirection * FrameTime, m_fMaxSpeed );
	m_vVelocity.x *= 0.95f;

	m_vPosition += m_vVelocity * FrameTime;
	m_vDirection = sf::Vector2f( 0.f, 0.f );
}

void PhyTest::AddImpulse( sf::Vector2f _vImpulse )
{
	m_vDirection += _vImpulse;
}

void PhyTest::AddVelocity( sf::Vector2f _vVelocity )
{
	m_vVelocity += _vVelocity;
}
#pragma endregion

#pragma region SplineTest
void SplineTest::Init()
{
	m_fCheckRadius = 150.f;
	m_iDraggedTarget = -1;

	m_oSpline.SetControlPointPosition( 0, sf::Vector2f( 0.f, 384.f ) );
	m_oSpline.SetControlPointPosition( 1, sf::Vector2f( 256.f, 768.f ) );
	m_oSpline.SetControlPointPosition( 2, sf::Vector2f( 512.f, 0.f ) );
	m_oSpline.SetControlPointPosition( 3, sf::Vector2f( 768.f, 768.f ) );
	m_oSpline.SetControlPointPosition( 4, sf::Vector2f( 1024.f, 384.f ) );

	/*m_bUp[0] = false;
	m_bUp[1] = true;
	m_bUp[2] = false;
	m_pSplineTargets[0] = sf::Vector2f( 256.f, 0.f );
	m_pSplineTargets[1] = sf::Vector2f( 512.f, 384.f );
	m_pSplineTargets[2] = sf::Vector2f( 768.f, 768.f );*/

	m_vTargetCheckerPos = sf::Vector2f( 0.f, 384.f );
}

void SplineTest::Update()
{
	/*for( int iTarget = 0 ; iTarget < 3 ; ++iTarget )
	{
		if( m_bUp[iTarget] )
		{
			if( m_pSplineTargets[iTarget].y <= 0.f )
				m_bUp[iTarget] = false;
			else
				m_pSplineTargets[iTarget] -= sf::Vector2f( 0.f, 1.f + iTarget );
		}
		else
		{
			if( m_pSplineTargets[iTarget].y > 768.f )
				m_bUp[iTarget] = true;
			else
			{
				m_pSplineTargets[iTarget] += sf::Vector2f( 0.f, 1.f + iTarget );
			}
		}
	}*/

	SplineTargetManagement();
	//SplineTargetDetection();
	BuildCircle();
}

void SplineTest::Display()
{
	//m_oSpline.DebugDraw();
	//m_oSpline2.DebugDraw();
	m_oSplineCenter.DebugDraw( true );

	sf::CircleShape oCircle( m_fCheckRadius );
	oCircle.setFillColor( sf::Color( 255, 255, 255, 70 ) );
	oCircle.setOutlineColor( sf::Color( 0, 255, 0, 70 ) );
	oCircle.setOrigin( sf::Vector2f( m_fCheckRadius, m_fCheckRadius ) );

	sf::CircleShape oEnnemy( SPLINE_TARGET_RADIUS );
	oEnnemy.setFillColor( sf::Color( 255, 0, 0, 150 ) );
	oEnnemy.setOrigin( sf::Vector2f( SPLINE_TARGET_RADIUS, SPLINE_TARGET_RADIUS ) );

	for( int iTarget = 0; iTarget < m_pSplineTargets.size(); ++iTarget )
	{
		oCircle.setPosition( m_pSplineTargets[ iTarget ] );
		oEnnemy.setPosition( m_pSplineTargets[ iTarget ] );
		g_pFZN_WindowMgr->Draw( oEnnemy );
		//g_pFZN_WindowMgr->Draw( oCircle );
	}
}

void SplineTest::SplineTargetManagement()
{
	sf::Vector2f vMousePos( FZN_Window->mapPixelToCoords( sf::Mouse::getPosition( *FZN_Window ) ) );

	if( g_pFZN_InputMgr->IsMouseDown( sf::Mouse::Left ) )
	{
		for( int iTarget = 0; iTarget < m_pSplineTargets.size(); ++iTarget )
		{
			if( m_iDraggedTarget == iTarget || fzn::Math::Square( SPLINE_TARGET_RADIUS ) >= fzn::Math::VectorLengthSq( m_pSplineTargets[ iTarget ] - vMousePos ) )
			{
				m_pSplineTargets[ iTarget ] = vMousePos;
				m_iDraggedTarget = iTarget;
				break;
			}
		}

		if( m_iDraggedTarget < 0 )
			m_pSplineTargets.push_back( vMousePos );
	}
	else if( g_pFZN_InputMgr->IsMousePressed( sf::Mouse::Right ) )
	{
		for( std::vector< sf::Vector2f >::iterator it = m_pSplineTargets.begin(); it != m_pSplineTargets.end(); ++it )
		{
			if( fzn::Math::Square( SPLINE_TARGET_RADIUS ) >= fzn::Math::VectorLengthSq( *it - vMousePos ) )
			{
				m_pSplineTargets.erase( it );
				break;
			}
		}
	}

	if( m_iDraggedTarget >= 0 && g_pFZN_InputMgr->IsMouseReleased( sf::Mouse::Left ) )
		m_iDraggedTarget = -1;
}

void SplineTest::SplineTargetDetection()
{
	m_vTargetCheckerPos = sf::Vector2f( 0.f, 384.f );	//Character position.
	sf::Vector2f vDirToEnd = sf::Vector2f( 1.f, 0.f );

	//m_oSpline.ClearPoints();
		//m_oSpline2.ClearPoints();
	m_oSplineCenter.ClearPoints();

	std::vector< fzn::HermiteCubicSpline::SplineControlPoint > oControlPoints;
	oControlPoints.push_back( fzn::HermiteCubicSpline::SplineControlPoint( sf::Vector2f( 50.f, 384.f )/*, sf::Vector2f( 75.f, 0.f ) */ ) );

	BuildSpline( oControlPoints );

	sf::Vector2f vDir = fzn::Math::VectorNormalization( oControlPoints.back().m_vPosition - oControlPoints.front().m_vPosition );
	m_vTargetCheckerPos = oControlPoints[ 0 ].m_vPosition + vDir;

	while( m_vTargetCheckerPos.x < 1024.f )
	{
		//m_vTargetCheckerPos = m_oSplineCenter.GetVertices()[ iVertex ].position;
		//CHeck every enemy.
		for( int iTarget = 0; iTarget < m_pSplineTargets.size(); ++iTarget )
		{
			if( m_pSplineTargets[ iTarget ].x < m_vTargetCheckerPos.x )
				continue;

			const sf::Vector2f vCheckToTarget = m_pSplineTargets[ iTarget ] - m_vTargetCheckerPos;

			if( fzn::Math::Square( m_fCheckRadius ) >= fzn::Math::VectorLengthSq( vCheckToTarget ) )
			{
				m_vTargetCheckerPos = m_pSplineTargets[ iTarget ];

				vDirToEnd = sf::Vector2f( 974.f, 384.f ) - m_vTargetCheckerPos;
				fzn::Math::VectorNormalize( vDirToEnd );

				oControlPoints.push_back( fzn::HermiteCubicSpline::SplineControlPoint( m_vTargetCheckerPos ) );
				BuildSpline( oControlPoints );

				/*const sf::VertexArray& oNewVertices = m_oSplineCenter.GetVertices();
				for( int iNewVertex = 0; iNewVertex < oNewVertices.getVertexCount(); ++iNewVertex )
				{
					if( oNewVertices[ iNewVertex ].position == m_vTargetCheckerPos )
					{
						iVertex = iNewVertex;
						break;
					}
				}*/
				break;
			}
		}

		m_vTargetCheckerPos += vDirToEnd;
	}

	BuildSpline( oControlPoints );
}

void SplineTest::BuildSpline( std::vector< fzn::HermiteCubicSpline::SplineControlPoint > _oControlPoints )
{
	m_oSplineCenter.ClearPoints();
	_oControlPoints.push_back( fzn::HermiteCubicSpline::SplineControlPoint( sf::Vector2f( 974.f, 384.f )/*, sf::Vector2f( 75.f, 0.f ) */ ) );

	fzn::HermiteCubicSpline::GenerateTangents( _oControlPoints );
	sf::Vector3f vZ( 0.f, 0.f, 1.f );

	for( int iControlPoint = 0; iControlPoint < _oControlPoints.size(); ++iControlPoint )
	{
		const sf::Vector2f& vTangent2D = _oControlPoints[ iControlPoint ].m_vTangent;
		const sf::Vector3f vTangent( vTangent2D.x, vTangent2D.y, 0.f );
		const sf::Vector3f vCross = fzn::Math::VectorCross( vTangent, vZ );
		const sf::Vector2f vCross2D = fzn::Math::VectorNormalization( { vCross.x, vCross.y } );

		//m_oSpline.AddPoint( sf::Vector2f( _oControlPoints[ iControlPoint ].m_vPosition + vCross2D * 16.f ) );
		//m_oSpline2.AddPoint( sf::Vector2f( _oControlPoints[ iControlPoint ].m_vPosition - vCross2D * 16.f ) );
		m_oSplineCenter.AddPoint( sf::Vector2f( _oControlPoints[ iControlPoint ].m_vPosition ) );
	}

	//m_oSpline.SetControlPointTangent( 0, sf::Vector2f( 75.f, 0.f ) );
	//m_oSpline2.SetControlPointTangent( 0, sf::Vector2f( 75.f, 0.f ) );
	m_oSplineCenter.SetControlPointTangent( 0, sf::Vector2f( 75.f, 0.f ) );

	//m_oSpline.Build( 1.f );
	//m_oSpline2.Build( 1.f );
	m_oSplineCenter.SetLoop( false );
	m_oSplineCenter.Build( 1.f );
}

void SplineTest::BuildCircle()
{
	m_oVertices.clear();
	m_oSplineCenter.ClearPoints();

	const sf::Vector2u vWindowSize = g_pFZN_WindowMgr->GetWindowSize();
	const float fRadius = 120.f;
	const sf::Vector2f vCirclePos = { vWindowSize.x * 0.5f, vWindowSize.y * 0.5f };

	std::vector< fzn::HermiteCubicSpline::SplineControlPoint > oControlPoints;

	for( int iPoint = 0; iPoint < 10; ++iPoint )
	{
		float fAngle = iPoint * 360.f / 10.f;
		fAngle = fzn::Math::DegToRad( fAngle );
		sf::Vector2f vPointPos;
		vPointPos.x = vCirclePos.x + cosf( fAngle ) * fRadius;
		vPointPos.y = vCirclePos.y + sinf( fAngle ) * fRadius;

		oControlPoints.push_back( fzn::HermiteCubicSpline::SplineControlPoint( vPointPos ) );
		//m_oSplineCenter.AddPoint( vPointPos );
	}

	CircleTargetDetection( oControlPoints, fRadius );

	for( const fzn::HermiteCubicSpline::SplineControlPoint& oControlPoint : oControlPoints )
	{
		m_oSplineCenter.AddPoint( oControlPoint.m_vPosition );
	}

	m_oSplineCenter.SetLoop( true );
	m_oSplineCenter.Build( 1.f );
}

void SplineTest::CircleTargetDetection( std::vector< fzn::HermiteCubicSpline::SplineControlPoint >& _oControlPoints, float _fCheckRadius )
{
	std::vector< std::pair< std::vector< fzn::HermiteCubicSpline::SplineControlPoint >::iterator, sf::Vector2f > > oTargets;

	struct CustomPoint
	{
		CustomPoint( const sf::Vector2f& _vPos, bool _bCustom )
			: m_vPos( _vPos )
			, m_bCustom( _bCustom )
		{}

		sf::Vector2f m_vPos = { 0.f, 0.f };
		bool m_bCustom = false;
	};

	std::map< float, CustomPoint > oMap;

	// Length of two vectors since we use prev to CP + CP to next.
	const float fLength = fzn::Math::VectorLength( _oControlPoints[ 1 ].m_vPosition - _oControlPoints[ 0 ].m_vPosition );

	//for( std::vector< fzn::HermiteCubicSpline::SplineControlPoint >::iterator it = _oControlPoints.begin(); it != _oControlPoints.end(); ++it )
	for( int iTarget = 0; iTarget < m_pSplineTargets.size(); ++iTarget )
	{
		int iClosestCP = -1;
		float fClosestDistance = FLT_MAX;

		for( int iCP = 0; iCP < _oControlPoints.size(); ++iCP )
		{
			const sf::Vector2f vCPToTarget = m_pSplineTargets[ iTarget ] - _oControlPoints[ iCP ].m_vPosition;
			const float fSquareDist = fzn::Math::VectorLengthSq( vCPToTarget );

			if( fzn::Math::Square( _fCheckRadius ) >= fSquareDist && fClosestDistance > fSquareDist )
			{
				iClosestCP = iCP;
				fClosestDistance = fSquareDist;
			}
		}

		if( iClosestCP < 0 )
			continue;

		int iNextCP = (iClosestCP + 1) % _oControlPoints.size();
		int iPrevCP = iClosestCP == 0 ? _oControlPoints.size() - 1 : iClosestCP - 1;

		const sf::Vector2f vCPToTarget = m_pSplineTargets[ iTarget ] - _oControlPoints[ iClosestCP ].m_vPosition;
		sf::Vector2f vPrevToNextCP = _oControlPoints[ iNextCP ].m_vPosition - _oControlPoints[ iPrevCP ].m_vPosition;
		vPrevToNextCP /= (fLength * 2.f);

		float fDot = fzn::Math::VectorDot( vPrevToNextCP, vCPToTarget );
		fDot /= fLength;

		oMap.emplace( std::make_pair( iClosestCP + fDot, CustomPoint( m_pSplineTargets[ iTarget ], true ) ) );
	}

	for( int iCP = 0; iCP < _oControlPoints.size(); ++iCP )
	{
		/*const float fCP = (float)iCP;
		std::map< float, CustomPoint >::iterator it = oMap.lower_bound( fCP );

		if( it != oMap.end() && ( it->first - fCP ) < 1.f )
		{
			it = oMap.upper_bound( (float)( iCP == 0 ? _oControlPoints.size() - 1 : iCP - 1 ) );

			float fItFirst = 0.f;

			if( it == oMap.end() )
			{
				it = oMap.begin();
				fItFirst = iCP == 0 ? it->first : 1.f - fabs( it->first ) + ( _oControlPoints.size() - 1 );
			}
			else
				fItFirst = it->first;

			if( it != oMap.end() && fCP > fItFirst && ( fCP - fItFirst ) < 1.f )
				continue;
		}*/

		oMap.emplace( std::make_pair( (float)iCP, CustomPoint( _oControlPoints[ iCP ].m_vPosition, false ) ) );
	}

	for( std::map< float, CustomPoint >::iterator it = oMap.begin(); it != oMap.end(); )
	{
		if( it->second.m_bCustom )
		{
			++it;
			continue;
		}

		std::map< float, CustomPoint >::iterator itPrev = std::prev( it == oMap.begin() ? oMap.end() : it );
		std::map< float, CustomPoint >::iterator itNext = it == std::prev( oMap.end() ) ? oMap.begin() : std::next( it );

		if( itPrev->second.m_bCustom && itNext->second.m_bCustom )
			it = oMap.erase( it );
		else
			++it;
	}

	_oControlPoints.clear();

	for( const std::pair< float, CustomPoint >& oPoint : oMap )
	{
		_oControlPoints.emplace_back( fzn::HermiteCubicSpline::SplineControlPoint( oPoint.second.m_vPos ) );
	}
}
#pragma endregion

#pragma region CollisionTest
void CollisionTest::Init()
{
	m_saOrientedBoundingBox.resize( 4 );
	m_saOrientedBoundingBox[ 0 ] = { 500.f, 500.f };
	m_saOrientedBoundingBox[ 1 ] = { 600.f, 600.f };
	m_saOrientedBoundingBox[ 2 ] = { 500.f, 700.f };
	m_saOrientedBoundingBox[ 3 ] = { 400.f, 600.f };
}

void CollisionTest::Update()
{
	auto OBBPointCollision = []( const sf::Shape& _daBox, const sf::Vector2f& _vPoint )
	{
		if( _daBox.getPointCount() < 3 )
			return false;

		auto Cross = [&]( const sf::Vector2f& _vA, const sf::Vector2f& _vB, const sf::Vector2f& _vPoint )
		{
			return ( _vB.x - _vA.x ) * ( _vPoint.y - _vA.y ) - ( _vB.y - _vA.y ) * ( _vPoint.x - _vA.x );
		};

		float fLastCross = Cross( _daBox.getPoint( 0 ), _daBox.getPoint( 1 ), _vPoint );
		int iBoxNextPoint = 0;

		for( int iBoxPoint = 1; iBoxPoint < _daBox.getPointCount(); ++iBoxPoint )
		{
			iBoxNextPoint = ( iBoxPoint + 1 ) % _daBox.getPointCount();
			float fCross = Cross( _daBox.getPoint( iBoxPoint ), _daBox.getPoint( iBoxNextPoint ), _vPoint );

			// If the multiplication results in a negative number, the two values have an opposite sign and so, the point is out of the bounding box.
			if( fCross * fLastCross < 0.f )
				return false;

			fLastCross = fCross;
		}

		return true;
	};

	sf::ConvexShape lol;
	OBBPointCollision( lol, { 0.f, 0.f } );

	sf::Vector2f vMousePos( FZN_Window->mapPixelToCoords( sf::Mouse::getPosition( *FZN_Window ) ) );

	if( g_pFZN_InputMgr->IsMouseDown( sf::Mouse::Left ) )
	{
		for( int iTarget = 0; iTarget < m_daTargets.size(); ++iTarget )
		{
			if( m_iDraggedTarget == iTarget || fzn::Math::Square( SPLINE_TARGET_RADIUS ) >= fzn::Math::VectorLengthSq( m_daTargets[ iTarget ].m_vPosition - vMousePos ) )
			{
				m_daTargets[ iTarget ].m_vPosition = vMousePos;
				m_iDraggedTarget = iTarget;
				break;
			}
		}

		if( m_iDraggedTarget < 0 )
			m_daTargets.push_back( { vMousePos, false } );
	}
	else if( g_pFZN_InputMgr->IsMousePressed( sf::Mouse::Right ) )
	{
		for( std::vector< Target >::iterator it = m_daTargets.begin(); it != m_daTargets.end(); ++it )
		{
			if( fzn::Math::Square( SPLINE_TARGET_RADIUS ) >= fzn::Math::VectorLengthSq( it->m_vPosition - vMousePos ) )
			{
				m_daTargets.erase( it );

				if( m_iDraggedTarget >= 0 )
					m_iDraggedTarget = -1;

				break;
			}
		}
	}

	if( m_iDraggedTarget >= 0 && g_pFZN_InputMgr->IsMouseReleased( sf::Mouse::Left ) )
		m_iDraggedTarget = -1;

	for( Target& rTarget : m_daTargets )
	{
		rTarget.m_bInBox = fzn::Tools::CollisionOBBPoint( m_saOrientedBoundingBox, rTarget.m_vPosition );
	}
}

void CollisionTest::Display()
{
	float fShapeRadius = 3.f;

	sf::CircleShape oShape( fShapeRadius );
	oShape.setFillColor( sf::Color::Yellow );
	oShape.setOrigin( sf::Vector2f( fShapeRadius, fShapeRadius ) );

	sf::VertexArray oLine( sf::PrimitiveType::LineStrip, 5 );
	oLine[ 0 ].color = sf::Color::White;
	oLine[ 1 ].color = sf::Color::White;
	oLine[ 2 ].color = sf::Color::White;
	oLine[ 3 ].color = sf::Color::White;
	oLine[ 4 ].color = sf::Color::White;

	for( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		oLine[ iPoint ].position = m_saOrientedBoundingBox[ iPoint ];
		oShape.setPosition( m_saOrientedBoundingBox[ iPoint ] );
		g_pFZN_WindowMgr->Draw( oShape );
	}

	oLine[ 4 ].position = oLine[ 0 ].position;
	g_pFZN_WindowMgr->Draw( oLine );

	fShapeRadius = 5.f;
	oShape.setRadius( fShapeRadius );
	oShape.setOrigin( { fShapeRadius, fShapeRadius } );

	for( const Target& rTarget : m_daTargets )
	{
		oShape.setFillColor( rTarget.m_bInBox ? sf::Color::Green : sf::Color::Red );
		oShape.setPosition( rTarget.m_vPosition );

		g_pFZN_WindowMgr->Draw( oShape );
	}
}
#pragma endregion

#pragma region LocalTests
void Common_LoadFileMemory( const char *name, void **buff, int *length )
{
	FILE *file = NULL;
	fopen_s( &file, name, "rb" );

	fseek( file, 0, SEEK_END );
	long len = ftell( file );
	fseek( file, 0, SEEK_SET );

	void *mem = malloc( len );
	fread( mem, 1, len, file );

	fclose( file );

	*buff = mem;
	*length = len;
}

void TESTFMOD()
{
	void* buffer = nullptr;
	void* buffer2 = nullptr;
	int iLength = 0, iLength2 = 0;
	Common_LoadFileMemory( DATAPATH( "Audio/Sounds/demoSound2.ogg" ), &buffer, &iLength );
	/*for( int iChar = 0 ; iChar < 30 ; ++iChar )
	{
		if( iChar > 26 )
			FZN_LOG( TRUE, TRUE, fzn::DBG_MSG_COL_WHITE, "|" );
		else if( iChar > 25 )
			FZN_LOG( TRUE, TRUE, fzn::DBG_MSG_COL_LIGHTGRAY, "|" );
		else if( iChar > 21 )
			FZN_LOG( TRUE, TRUE, fzn::DBG_MSG_COL_GRAY, "|" );
		else if( iChar > 17 )
			FZN_LOG( TRUE, TRUE, fzn::DBG_MSG_COL_PURPLE, "|" );
		else if( iChar > 13 )
			FZN_LOG( TRUE, TRUE, fzn::DBG_MSG_COL_BLUE, "|" );
		else if( iChar > 5 )
			FZN_LOG( TRUE, TRUE, fzn::DBG_MSG_COL_GREEN, "|" );
		else if( iChar > 4 )
			FZN_LOG( TRUE, TRUE, fzn::DBG_MSG_COL_YELLOW, "|" );
		else if( iChar > 3 )
			FZN_LOG( TRUE, TRUE, fzn::DBG_MSG_COL_DARKYELLOW, "|" );
		else if( iChar >= 0 )
			FZN_LOG( TRUE, TRUE, fzn::DBG_MSG_COL_RED, "|" );
	}*/
	printf( "\n" );
	for( int iChar = 0 ; iChar < 30 ; ++iChar )
	{
		printf( "%c", ( (char*)buffer )[iChar] );
	}
	printf( "\n" );
	Common_LoadFileMemory( DATAPATH( "Audio/Sounds/Passage.ogg" ), &buffer2, &iLength2 );
	for( int iChar = 0 ; iChar < 30 ; ++iChar )
	{
		printf( "%c", ( (char*)buffer2 )[iChar] );
	}

	std::string sHeader, sFile1, sFile2;

	sHeader = std::string( (char*)buffer, 35 );

	sFile1 = std::string( (char*)buffer, iLength );
	//sFile1 = sFile1.substr( 35 );

	sFile2 = std::string( (char*)buffer2, iLength2 );
	sFile2 = sFile2.substr( 35 );

	printf( "\n\n" );
	for( int iChar = 0 ; iChar < 3000 ; ++iChar )
	{
		printf( "%c", sFile1[iChar] );
	}

	printf( "\n\n%s\n%s\n%s\n", sHeader.c_str(), sFile1.c_str(), sFile2.c_str() );
}
#pragma endregion

/////////////////CONSTRUCTOR / DESTRUCTOR/////////////////

//------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Default constructor
//------------------------------------------------------------------------------------------------------------------------------------------------------------------
Demo::Demo()
{
	//Add the callbacks you'll need in the game loop so your functions can be called (there are three type of callbacks : Update, Display or Event; choose wisely)
	g_pFZN_Core->AddCallback( this, &Demo::Update, fzn::DataCallbackType::Update );
	g_pFZN_Core->AddCallback( this, &Demo::Display, fzn::DataCallbackType::Display );

	g_pFZN_DataMgr->LoadResourceGroup( "demo" );		//Loading of the resources corresponding to the demonstration group (see the Resource file in Data/XMLFiles)

	bnz = *g_pFZN_DataMgr->GetAnimation( "BnZ" );		//Recuperation of the demonstration animation from de DataManager (Animations are in "play" status by default)
	bnz.SetPosition( sf::Vector2f( 0.f, 300.f ) );
	bnz.Play( true );

	wumpa = *g_pFZN_DataMgr->GetAnimation( "Wumpa" );

	wumpa.SetPosition( sf::Vector2f( 480.f, 5.f ) );

	sound.setBuffer( *g_pFZN_DataMgr->GetSoundBuffer( "demoSound" ) );						//Recuperation of the demonstration sound from the DataManager
	sound.play();

	//g_pFZN_DataMgr->LoadAnm2s( "../../Data/Display/Animations/001.000_player.anm2" );
	//isek = *g_pFZN_DataMgr->GetAnm2( "Cursor" );
	//isek.Play();
	//.SetPosition( sf::Vector2f( 66.f, 140.f ) );

	/*walkRight = *g_pFZN_DataMgr->GetAnimation( "WalkRight" );
	walkRight.SetPosition( sf::Vector2f( 50.f, 132.f ) );
	walkRight.Play();*/
	

	//TESTFMOD();

	music = g_pFZN_DataMgr->GetSfMusic( "demoMusic" );							//Recuperation of the demonstration music
	//music->play();

	text.setFont( *g_pFZN_DataMgr->GetFont( "defaultFont" ) );						//Recuperation of the default font (not part of the demo resource group, charged in the main when the resource file has been loaded)
	text.setString( "Welcome to the FaZoN template !" );
	text.setPosition( bnz.GetPosition() + sf::Vector2f( 85.f, 0.f ) );

	sprite.setTexture( *g_pFZN_DataMgr->GetTexture( "demoSprite" ) );				//Recuperation of the demonstration sprite
	sprite.setPosition( sf::Vector2f( (float)FZN_Window->getSize().x - sprite.getTexture()->getSize().x, 0.f ) );
	//sprite.setRotation( 90.f );

	fToFlip.setFont( *g_pFZN_DataMgr->GetFont( "defaultFont" ) );
	fToFlip.setString( "Press F to flip !" );
	fToFlip.setPosition( sprite.getPosition() + sf::Vector2f( -5.f, (float)sprite.getTexture()->getSize().y ) );

	progressBar.SetSize( sf::Vector2f( 200.f, 10.f ) );
	progressBar.SetPosition( sf::Vector2f( 50.f, 200.f ) );
	progressBar.SetMaxValue( (float)music->getDuration().asMilliseconds() );
	progressBar.SetCurrentValue( (float)music->getDuration().asMilliseconds() );

	m_phyTest.m_fMaxSpeed = 1000.f;
	m_phyTest.m_vMaxValues = sf::Vector2f( 300.f, 2000.f );
	m_phyTest.m_bUseHorizontalMaxValues = true;

	//m_oSplineTest.Init();
	m_oCollisionTest.Init();
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Destructor
//------------------------------------------------------------------------------------------------------------------------------------------------------------------
Demo::~Demo()
{
	g_pFZN_DataMgr->UnloadResourceGroup( "demo" );
}


/////////////////OTHER FUNCTIONS/////////////////

//------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Update of the demonstration
//------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Demo::Update()
{
	progressBar.Update();

	if( g_pFZN_InputMgr->IsMouseOver( text ) )						//Test if the mouse cursor is on an element, overloads available with a sprite, an animation.
		text.setFillColor( sf::Color::Green );
	else text.setFillColor( sf::Color::White );

	if( sound.getStatus() == sf::Sound::Status::Stopped && music->getStatus() == sf::Music::Status::Stopped )
	{
		music->play();
	}	

	if( g_pFZN_InputMgr->IsActionPressed( "Flip" ) )		//Single press test on a key, SFML offers only full press or up, two more states are possible with the inputManager resulting in : Up, Pressed, Down, Released.
	{
		//isek.Play();
		//walkRight.Play();
		fzn::Tools::SpriteFlipX( sprite );
	}

	if( m_phyTest.m_vPosition.y > 700.f && m_phyTest.m_vVelocity.y > 0.f )
	{
		m_phyTest.m_vVelocity.y = 0.f;
		//m_phyTest.AddImpulse(-m_phyTest.m_vVelocity / FrameTimeS);
		m_phyTest.m_vPosition.y = 700.f;
	}
	//else m_phyTest.AddImpulse(m_phyTest.m_vGravity );

	if( g_pFZN_InputMgr->IsActionPressed( "Jump" ) )
	{
		m_phyTest.AddVelocity( sf::Vector2f( 0.f, -500.f ) );
		//m_phyTest.AddImpulse(sf::Vector2f(0.f, -25000.f));
	}
	if( g_pFZN_InputMgr->IsActionPressed( "Right" ) || g_pFZN_InputMgr->IsActionDown( "Right" ) )
		m_phyTest.AddVelocity( sf::Vector2f( 10.f, 0.f ) );
	else if( g_pFZN_InputMgr->IsActionPressed( "Left" ) || g_pFZN_InputMgr->IsActionDown( "Left" ) )
		m_phyTest.AddVelocity( sf::Vector2f( -10.f, 0.f ) );

	//m_phyTest.Update();
	//m_oSplineTest.Update();
	m_oCollisionTest.Update();

	//printf("POSITION %f %f\n", m_phyTest.m_vPosition.x, m_phyTest.m_vPosition.y);

	wumpa.SetPosition( m_phyTest.m_vPosition );
	
	//ImGui::Begin("plop");
	//ImGui::End();

	if( g_pFZN_InputMgr->IsKeyPressed( sf::Keyboard::Escape ) )
		g_pFZN_Core->QuitApplication();
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Display of the demonstration
//------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Demo::Display()
{
	//m_jumpTest.Update();
	g_pFZN_WindowMgr->Draw( bnz );		//Draw calls are made through the Graphic Singleton, any SFML drawable will work, animations too
	g_pFZN_WindowMgr->Draw( wumpa );
	g_pFZN_WindowMgr->Draw( walkRight );
	//g_pFZN_WindowMgr->Draw( isek );
	g_pFZN_WindowMgr->Draw( text );
	g_pFZN_WindowMgr->Draw( sprite );
	g_pFZN_WindowMgr->Draw( fToFlip );
	g_pFZN_WindowMgr->Draw( progressBar );

	fzn::Tools::DrawLine( sf::Vector2f( 0.f, 384.f ), sf::Vector2f( 1024.f, 384.f ), sf::Color( 150, 150, 150, 150 ) );

	//m_oSplineTest.Display();
	m_oCollisionTest.Display();
}
