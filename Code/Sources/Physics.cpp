#include <FZN/Managers/FazonCore.h>
#include <FZN/Managers/WindowManager.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Math.h>

#include <SFML/Graphics/CircleShape.hpp>

#include "Physics.h"

PhysicsTest* g_pPhysics = nullptr;

//=======================================================
/// MATH FUNCTIONS TO MOVE IN FZN
//=======================================================
#pragma region Math
bool VectorsSameDirection( const sf::Vector2f& _vVectorA, const sf::Vector2f& _vVectorB )
{
	return fzn::Math::VectorDot( _vVectorA, _vVectorB ) >= 0.f;
}

bool VectorsPerpendicular( const sf::Vector2f& _vVectorA, const sf::Vector2f& _vVectorB )
{
	return fzn::Math::VectorDot( _vVectorA, _vVectorB ) == 0.f;
}

sf::Vector2f GetPerpendicularVector( const sf::Vector2f& _vSegment, const sf::Vector2f& _vDirection )
{
	sf::Vector3f vSegment3D{ _vSegment.x, _vSegment.y, 0.f };
	sf::Vector3f vCross = fzn::Math::VectorCross( vSegment3D, { _vDirection.x, _vDirection.y, 0.f } );
	sf::Vector3f vResult3D = fzn::Math::VectorCross( vCross, vSegment3D );

	return { vResult3D.x, vResult3D.y };
}

sf::Vector2f GetFarthestPointInDirection( const sf::Shape* _pShape, const sf::Vector2f& _vDirection )
{
	if( _pShape == nullptr )
		return { 0.f, 0.f };

	float fMaxProj = -FLT_MAX;
	float fDot{ 0.f };
	sf::Vector2f vFarthestPoint{ 0.f, 0.f };
	sf::Vector2f vCurrentPoint{ 0.f, 0.f };

	const sf::Transform& rTransform = _pShape->getTransform();

	for( int iPoint = 0; iPoint < _pShape->getPointCount(); ++iPoint )
	{
		vCurrentPoint = rTransform.transformPoint( _pShape->getPoint( iPoint ) );
		fDot = fzn::Math::VectorDot( _vDirection, vCurrentPoint );

		if( fMaxProj < fDot )
		{
			fMaxProj = fDot;
			vFarthestPoint = vCurrentPoint;
		}
	}

	return vFarthestPoint;
}

sf::Vector2f GetSupportPoint( const sf::Shape* _pShapeA, const sf::Shape* _pShapeB, const sf::Vector2f& _vDirection )
{
	if( _pShapeA == nullptr || _pShapeB == nullptr )
		return { 0.f, 0.f };

	// Get points on the edge of the shapes in opposite directions
	const sf::Vector2f vPointA = GetFarthestPointInDirection( _pShapeA, _vDirection );
	const sf::Vector2f vPointB = GetFarthestPointInDirection( _pShapeB, -_vDirection );

	return vPointA - vPointB;
}

// Returns true if intersection
bool DoSimplex( std::vector< sf::Vector2f >& _vSimplex, sf::Vector2f& _vDirection )
{
	auto DoSimplexLine = []( std::vector< sf::Vector2f >&_vSimplex, sf::Vector2f & _vDirection )
	{
		const sf::Vector2f& vNewestPoint = _vSimplex.back();
		const sf::Vector2f vToOrigin = -vNewestPoint;
		const sf::Vector2f vLine = _vSimplex[ 0 ] - vNewestPoint;

		if( VectorsSameDirection( vLine, vToOrigin ) )
		{
			_vDirection = GetPerpendicularVector( vLine, vToOrigin );
		}
		else
			_vDirection = vToOrigin;
	};

	auto DoSimplexTriangle = []( std::vector< sf::Vector2f >& _vSimplex, sf::Vector2f& _vDirection ) -> bool
	{
		std::vector< sf::Vector2f > vTriangle( 3 );
		const sf::Vector2f& vNewestPoint = _vSimplex.back();
		const sf::Vector2f& vPreviousPoint1 = _vSimplex[ _vSimplex.size() - 2 ];
		const sf::Vector2f& vPreviousPoint2 = _vSimplex[ _vSimplex.size() - 3 ];
		vTriangle[ 0 ] = vPreviousPoint1;
		vTriangle[ 1 ] = vPreviousPoint2;
		vTriangle[ 2 ] = vNewestPoint;

		// Is the origin in the current triangle
		if( fzn::Tools::CollisionOBBPoint( vTriangle, { 0.f, 0.f } ) )
			return true;

		const sf::Vector2f vToOrigin = -vNewestPoint;
		const sf::Vector2f vLine1 = vPreviousPoint1 - vNewestPoint;
		const sf::Vector2f vLine2 = vPreviousPoint2 - vNewestPoint;

		const sf::Vector3f vLine1_3D{ vLine1.x, vLine1.y, 0.f };
		const sf::Vector3f vLine2_3D{ vLine2.x, vLine2.y, 0.f };

		const sf::Vector3f vPlane = fzn::Math::VectorCross( vLine1_3D, vLine2_3D );
		sf::Vector3f vNormale3D = fzn::Math::VectorCross( vPlane, vLine1_3D );
		sf::Vector2f vNormale2D{ vNormale3D.x, vNormale3D.y };

		// ABC x AB . AO && AB . AO
		if( VectorsSameDirection( vNormale2D, vToOrigin ) && VectorsSameDirection( vLine1, vToOrigin ) )
		{
			_vDirection = GetPerpendicularVector( vLine1, vToOrigin );
			return false;
		}
		else
		{
			vNormale3D = fzn::Math::VectorCross( vPlane, vLine2_3D );
			vNormale2D = { vNormale3D.x, vNormale3D.y };

			// ABC x AC . AO && AC . AO
			if( VectorsSameDirection( vNormale2D, vToOrigin ) && VectorsSameDirection( vLine2, vToOrigin ) )
			{
				_vDirection = GetPerpendicularVector( vLine2, vToOrigin );
				return false;
			}
		}

		_vDirection = vToOrigin;
		return false;
	};

	if( _vSimplex.size() < 2 )
		return false;

	if( _vSimplex.size() == 2 )
	{
		DoSimplexLine( _vSimplex, _vDirection );
		return false;
	}
	else
		return DoSimplexTriangle( _vSimplex, _vDirection );
}
#pragma endregion

#pragma region Tools
bool GJKIntersection( std::vector< sf::Vector2f >& _vSimplex, const sf::Shape* _pShapeA, const sf::Shape* _pShapeB )
{
	//std::vector< sf::Vector2f > vSimplex;
	sf::Vector2f vSupportPoint = GetSupportPoint( _pShapeA, _pShapeB, { 1.f, 0.f } );
	sf::Vector2f vDirection = -vSupportPoint;

	_vSimplex.push_back( vSupportPoint );

	bool bLookForIntersection = true;

	while( bLookForIntersection )
	{
		sf::Vector2f vNewPoint = GetSupportPoint( _pShapeA, _pShapeB, vDirection );

		if( VectorsSameDirection( vDirection, vNewPoint ) == false )
			return false;

		_vSimplex.push_back( vNewPoint );

		if( DoSimplex( _vSimplex, vDirection ) )
			return true;

		if( _vSimplex.size() >= 30 )
		{
			FZN_LOG( "SIMPLEX FULL !!!!!!!!!!!!!!!!!!!" );
			return false;
		}
	}

	return false;
}
#pragma endregion


RigidBody::RigidBody()
{
	
}

void RigidBody::Update( float _fGravity, const std::vector< RigidBody* >& _daRigidBodies )
{
	if( m_bUpdateLastPos )
		m_vLastPos = GetPosition();

	//SetPosition( GetPosition() + m_vVelocity * FrameTime );

	_ComputeMovementBounds();

	if( m_bUpdateLastPos )
		fLastPosLength = fzn::Math::VectorLength( { GetPosition() - m_vLastPos } );
}

void RigidBody::Display()
{
	if( m_pShape == nullptr )
		return;

	g_pFZN_WindowMgr->Draw( *m_pShape );

	fzn::Tools::DrawLine( m_pShape->getPosition(), m_pShape->getPosition() + m_vLastImpulse, sf::Color::Green );
	fzn::Tools::DrawLine( m_pShape->getPosition(), m_pShape->getPosition() + m_vVelocity * FrameTime, sf::Color::Cyan );

	int iNextPoint{ 0 };
	sf::Vector2f vMiddlePoint{ 0.f, 0.f };
	sf::Vector2f vPointA{ 0.f, 0.f };
	sf::Vector2f vPointB{ 0.f, 0.f };

	for( int iPoint = 0; iPoint < m_pShape->getPointCount(); ++iPoint )
	{
		iNextPoint = ( iPoint + 1 ) % m_pShape->getPointCount();

		vPointA = m_pShape->getTransform().transformPoint( m_pShape->getPoint( iPoint ) );
		vPointB = m_pShape->getTransform().transformPoint( m_pShape->getPoint( iNextPoint ) );
		vMiddlePoint.x = ( vPointA.x + vPointB.x ) / 2.f;
		vMiddlePoint.y = ( vPointA.y + vPointB.y ) / 2.f;

		fzn::Tools::DrawLine( vMiddlePoint, vMiddlePoint + m_aNormals[ iPoint ] * 3.f, sf::Color::Magenta );

		fzn::Tools::DrawString( fzn::Tools::Sprintf( "%.0f;%.0f", vPointA.x, vPointA.y ).c_str(), vPointA, 12 );
	}

	sf::RectangleShape oNextPosBounds( { m_oMouvementBounds.width, m_oMouvementBounds.height } );
	oNextPosBounds.setFillColor( { 0, 0, 0, 0 } );
	oNextPosBounds.setOutlineColor( { 255, 255, 255, 200 } );
	oNextPosBounds.setOutlineThickness( 1.f );
	oNextPosBounds.setPosition( m_oMouvementBounds.left, m_oMouvementBounds.top );

	/*auto rGlobalBOunds = m_pShape->getGlobalBounds();

	oNextPosBounds.setSize( { rGlobalBOunds.width, rGlobalBOunds.height } );
	oNextPosBounds.setPosition( rGlobalBOunds.left, rGlobalBOunds.top );*/

	//g_pFZN_WindowMgr->Draw( oNextPosBounds );
}

void RigidBody::OnEvent()
{
	const fzn::Event& rFznEvent = g_pFZN_Core->GetEvent();

	if( rFznEvent.m_eType == fzn::Event::eUserEvent )
	{
		PhysicsEvent* pPhyEvent = static_cast<PhysicsEvent*>( rFznEvent.m_pUserData );
		
		if( pPhyEvent == nullptr )
			return;

		if( pPhyEvent->m_oCollisionEvent.m_pRigidBodyA == this || pPhyEvent->m_oCollisionEvent.m_pRigidBodyB == this )
			_OnCollision( pPhyEvent->m_oCollisionEvent.m_oCollisionPoint );
	}
}

bool RigidBody::IsHovered( const sf::Vector2f& _vMousePos )
{
	return fzn::Tools::CollisionOBBPoint( *m_pShape, _vMousePos );
}

bool RigidBody::IsColliding( const RigidBody& _rRigiBody, CollisionPoint& _rCollisionPoint ) const
{
	if( fzn::Tools::CollisionAABBAABB( m_oMouvementBounds, _rRigiBody.m_oMouvementBounds ) == false )
		return false;

	sf::ConvexShape oSweptBounds( m_pShape->getPointCount() + 2 );	// Adding two points because they will be the ones holing the swept bounds together

	const sf::Vector2f vBasePosition{ GetPosition() };
	const sf::Vector2f vNextPosition{ vBasePosition + m_vVelocity * FrameTime };

	const sf::FloatRect oBaseBounds{ m_pShape->getGlobalBounds() };

	sf::ConvexShape oNextShape = std::move( fzn::Tools::ConvertShapePtrToConvexShape( m_pShape ) );
	sf::ConvexShape oShape( oNextShape );

	oNextShape.setPosition( vNextPosition );

	const sf::Vector2f vTranslation = oNextShape.getPosition() - oShape.getPosition();

	/*
	* vMiddlePoint.x = ( vPointA.x + vPointB.x ) / 2.f;
	vMiddlePoint.y = ( vPointA.y + vPointB.y ) / 2.f;
	*/

	const sf::Vector2f vMiddlePoint{ ( vBasePosition.x + vNextPosition.x ) / 2.f, ( vBasePosition.y + vNextPosition.y ) / 2.f };
	const sf::Vector2f vTranslationNormal{ vTranslation.y, -vTranslation.x };

	const sf::Transform& rTransform = oShape.getTransform();

	auto GetStartEndPoints = []( const sf::Vector2f& vNormal, const sf::ConvexShape& _rShape, int& _iStartPoint, int& _iEndPoint )
	{
	};

	/*for( int iPoint = 0; iPoint < oShape.getPointCount(); ++iPoint )
	{
		fDot = fzn::Math::VectorDot( _vNormal, rTransform.transformPoint( oShape.getPoint( iPoint ) ) );

		SupThenAffect( _fMinProj, fDot );
		InfThenAffect( _fMaxProj, fDot );
	}*/
}

bool RigidBody::IsColliding_SAT( const RigidBody& _rRigiBody, CollisionPoint& _rCollisionPoint ) const
{
	float fThisBodyMinProjection = FLT_MAX;
	float fThisBodyMaxProjection = -FLT_MAX;
	float fOtherBodyMinProjection = FLT_MAX;
	float fOtherBodyMaxProjection = -FLT_MAX;

	auto SATtest = []( const sf::Vector2f& _vNormal, const sf::Shape* _pShape, float& _fMinProj, float& _fMaxProj )
	{
		_fMinProj = FLT_MAX;
		_fMaxProj = -FLT_MAX;
		float fDot{ 0.f };

		const sf::Transform& rTransform = _pShape->getTransform();
		
		for( int iPoint = 0; iPoint < _pShape->getPointCount(); ++iPoint )
		{
			fDot = fzn::Math::VectorDot( _vNormal, rTransform.transformPoint( _pShape->getPoint( iPoint ) ) );

			SupThenAffect( _fMinProj, fDot );
			InfThenAffect( _fMaxProj, fDot );
		}
	};

	auto IsBetweenOrdered = []( float _fValue, float _fLowerBound, float _fUpperBound ) -> bool
	{
		return _fLowerBound <= _fValue && _fValue <= _fUpperBound;
	};

	auto Overlaps = [&]( float _fMin1, float _fMax1, float _fMin2, float _fMax2 ) -> bool
	{
		return IsBetweenOrdered( _fMin2, _fMin1, _fMax1 ) || IsBetweenOrdered( _fMin1, _fMin2, _fMax2 );
	};

	auto CheckNormals = [&]( const std::vector< sf::Vector2f >& _rNormals ) -> bool
	{
		for( const sf::Vector2f& rNormal : _rNormals )
		{
			SATtest( rNormal, m_pShape, fThisBodyMinProjection, fThisBodyMaxProjection );
			SATtest( rNormal, _rRigiBody.m_pShape, fOtherBodyMinProjection, fOtherBodyMaxProjection );

			if( Overlaps( fThisBodyMinProjection, fThisBodyMaxProjection, fOtherBodyMinProjection, fOtherBodyMaxProjection ) == false )
				return false;
		}

		return true;
	};

	if( CheckNormals( m_aNormals ) == false )
		return false;

	if( CheckNormals( _rRigiBody.m_aNormals ) == false )
		return false;

	_rCollisionPoint.m_vCollisionResponse = -m_vVelocity;
}

void RigidBody::AddImpulse( const sf::Vector2f& _vImpulse )
{
	m_vVelocity += _vImpulse;
	m_vLastImpulse = _vImpulse;
}

void RigidBody::SetPosition( const sf::Vector2f& _vPosition )
{
	if( m_pShape != nullptr )
		m_pShape->setPosition( _vPosition );
}

sf::Vector2f RigidBody::GetPosition() const
{
	return m_pShape != nullptr ? m_pShape->getPosition() : sf::Vector2f{};
}

bool RigidBody::HasVelocity() const
{
	return fzn::Math::VectorLengthSq( m_vVelocity ) > 0.f;
}

sf::FloatRect RigidBody::GetGlobalBounds() const
{
	if( m_pShape == nullptr )
		return sf::FloatRect();

	return m_pShape->getGlobalBounds();
}

void RigidBody::_OnCollision( const CollisionPoint& _rCollision )
{
	m_vVelocity += _rCollision.m_vCollisionResponse;

	if( m_bUpdateLastPos )
	{
		FZN_LOG( "COLLISION %f", _rCollision.m_vContactPoint.y );
		m_bUpdateLastPos = false;
	}
}

void RigidBody::_ComputeVelocity( float _fGravity )
{
	m_vVelocity += sf::Vector2f( 0.f, _fGravity ) * FrameTime;
}

void RigidBody::_ComputeNormals()
{
	if( m_pShape == nullptr )
		return;

	m_aNormals.resize( m_pShape->getPointCount() );

	int iNextPoint{ 0 };
	sf::Vector2f vSegment{ 0.f, 0.f };
	sf::Vector2f vPointA{ 0.f, 0.f };
	sf::Vector2f vPointB{ 0.f, 0.f };

	for( int iPoint = 0; iPoint < m_pShape->getPointCount(); ++iPoint )
	{
		iNextPoint = ( iPoint + 1 ) % m_pShape->getPointCount();

		vPointA = m_pShape->getPoint( iPoint );
		vPointB = m_pShape->getPoint( iNextPoint );
		vSegment = vPointB - vPointA;

		m_aNormals[ iPoint ].x = vSegment.y;
		m_aNormals[ iPoint ].y = vSegment.x * -1.f;
	}
}

void RigidBody::_ComputeMovementBounds()
{
	const sf::Vector2f vBasePosition{ GetPosition() };

	const sf::FloatRect oBaseBounds{ m_pShape->getGlobalBounds() };

	m_pShape->setPosition( vBasePosition + m_vVelocity * FrameTime );

	const sf::FloatRect oNextPosBounds{ m_pShape->getGlobalBounds() };

	const sf::FloatRect* pLeftMostBounds{ nullptr };
	const sf::FloatRect* pTopMostBounds{ nullptr };
	const sf::FloatRect* pRightMostBounds{ nullptr };
	const sf::FloatRect* pBottomMostBounds{ nullptr };

	if( oBaseBounds.left < oNextPosBounds.left )
	{
		pLeftMostBounds = &oBaseBounds;
		pRightMostBounds = &oNextPosBounds;
	}
	else
	{
		pLeftMostBounds = &oNextPosBounds;
		pRightMostBounds = &oBaseBounds;
	}

	if( oBaseBounds.top < oNextPosBounds.top )
	{
		pTopMostBounds = &oBaseBounds;
		pBottomMostBounds = &oNextPosBounds;
	}
	else
	{
		pTopMostBounds = &oNextPosBounds;
		pBottomMostBounds = &oBaseBounds;
	}

	m_oMouvementBounds.left = pLeftMostBounds->left;
	m_oMouvementBounds.top = pTopMostBounds->top;
	m_oMouvementBounds.width = (pRightMostBounds->left - pLeftMostBounds->left) + pRightMostBounds->width;
	m_oMouvementBounds.height = ( pBottomMostBounds->top - pTopMostBounds->top ) + pBottomMostBounds->height;

	m_pShape->setPosition( vBasePosition );
}

Wheel::Wheel()
{
	m_pShape = new sf::CircleShape( 15.f );
	m_pShape->setFillColor( { 0, 0, 0, 0 } );
	m_pShape->setOutlineColor( sf::Color::White );
	m_pShape->setOutlineThickness( 1.f );

	m_oLastPosCircle.setRadius( 15.f );
	m_oLastPosCircle.setFillColor( { 0, 0, 0, 0 } );
	m_oLastPosCircle.setOutlineColor( { 255, 0, 0, 200 } );
	m_oLastPosCircle.setOutlineThickness( 1.f );
	m_oLastPosCircle.setOrigin( { 15.f, 15.f } );

	if( sf::CircleShape* pCircleShape = static_cast< sf::CircleShape* >( m_pShape ) )
		m_pShape->setOrigin( { pCircleShape->getRadius(), pCircleShape->getRadius() } );

	_ComputeNormals();

	g_pFZN_Core->AddCallback( this, &Wheel::OnEvent, fzn::DataCallbackType::Event );
}

Wheel::Wheel( const Wheel& _rWheel )
{
	m_pShape = new sf::CircleShape( 15.f );
	m_pShape->setFillColor( { 0, 0, 0, 0 } );
	m_pShape->setOutlineColor( sf::Color::White );
	m_pShape->setOutlineThickness( 1.f );

	m_oLastPosCircle.setRadius( 15.f );
	m_oLastPosCircle.setFillColor( { 0, 0, 0, 0 } );
	m_oLastPosCircle.setOutlineColor( { 255, 0, 0, 200 } );
	m_oLastPosCircle.setOutlineThickness( 1.f );
	m_oLastPosCircle.setOrigin( { 15.f, 15.f } );

	if( sf::CircleShape* pCircleShape = static_cast<sf::CircleShape*>( m_pShape ) )
		m_pShape->setOrigin( { pCircleShape->getRadius(), pCircleShape->getRadius() } );

	SetPosition( _rWheel.GetPosition() );
	m_vVelocity = _rWheel.m_vVelocity;
	m_vLastImpulse = _rWheel.m_vLastImpulse;
	m_aNormals = _rWheel.m_aNormals;

	g_pFZN_Core->AddCallback( this, &Wheel::OnEvent, fzn::DataCallbackType::Event );
}

Wheel::~Wheel()
{
	g_pFZN_Core->RemoveCallback( this, &Wheel::OnEvent, fzn::DataCallbackType::Event );
}

void Wheel::Update( float _fGravity, const std::vector< RigidBody* >& _daRigidBodies )
{
	RigidBody::Update( _fGravity, _daRigidBodies );

	m_oLastPosCircle.setPosition( m_vLastPos );
}

void Wheel::Display()
{
	g_pFZN_WindowMgr->Draw( m_oLastPosCircle );

	RigidBody::Display();

	fzn::Tools::DrawLine( m_pShape->getPosition() + m_pShape->getPoint( 0 ), m_pShape->getPosition(), sf::Color::Red );
}

bool Wheel::IsHovered( const sf::Vector2f& _vMousePos )
{
	return fzn::Tools::CollisionCirclePoint( *GetShape< sf::CircleShape* >(), _vMousePos );
}

Ground::Ground()
{
	m_pShape = new sf::ConvexShape( 4 );

	m_pShape->setFillColor( { 255, 255, 255, 100 } );
	m_pShape->setOutlineColor( sf::Color::White );
	m_pShape->setOutlineThickness( 1.f );

	if( sf::ConvexShape* pConvex = GetShape< sf::ConvexShape* >() )
	{
		const sf::Vector2u& vWindowSize = g_pFZN_WindowMgr->GetWindowSize();

		pConvex->setPoint( 0, { 0.f, 700.f } );
		pConvex->setPoint( 1, { (float)vWindowSize.x, 700.f } );
		pConvex->setPoint( 2, { (float)vWindowSize.x, (float)vWindowSize.y } );
		pConvex->setPoint( 3, { 0.f, (float)vWindowSize.y } );
	}

	_ComputeNormals();

	g_pFZN_Core->AddCallback( this, &Ground::OnEvent, fzn::DataCallbackType::Event );
}

TestShape::TestShape()
{
	m_pShape = new sf::CircleShape( 80.f, 9 );

	m_pShape->setFillColor( { 255, 255, 255, 100 } );
	m_pShape->setOutlineColor( sf::Color::White );
	m_pShape->setOutlineThickness( 1.f );

	if( sf::CircleShape* pCircleShape = static_cast<sf::CircleShape*>( m_pShape ) )
		m_pShape->setOrigin( { pCircleShape->getRadius(), pCircleShape->getRadius() } );

	_ComputeNormals();
}

PhysicsTest::PhysicsTest()
{
	g_pFZN_Core->AddCallback( this, &PhysicsTest::Update, fzn::DataCallbackType::Update );
	g_pFZN_Core->AddCallback( this, &PhysicsTest::Display, fzn::DataCallbackType::Display );

	m_daRigidBodies.push_back( &m_oGround );

	g_pPhysics = this;
}

void PhysicsTest::Update()
{
	m_daSimplex.clear();

	const float fGravity = 981.f;
	sf::Vector2f vMousePos = g_pFZN_WindowMgr->GetMousePosition();

	for( Wheel* pWheel : m_aWheels )
	{
		if( pWheel == m_pDraggedWheel )
			continue;
	
		//pWheel->_ComputeVelocity( fGravity );
	}

	_CollisionDetection();

	for( RigidBody* pWheel : m_daRigidBodies )
	{
		if( pWheel == m_pDraggedWheel )
			continue;


		pWheel->Update( fGravity, m_daRigidBodies );
		/*if( m_oGround.IsColliding( rWheel, m_oCollisionPoint ) )
		{
			PhysicsEvent* pEvent = new PhysicsEvent( PhysicsEvent::Type::Collision );
			pEvent->m_oCollisionEvent.m_oCollisionPoint = m_oCollisionPoint;
			pEvent->m_oCollisionEvent.m_pRigidBodyA = &rWheel;
			pEvent->m_oCollisionEvent.m_pRigidBodyB = &m_oGround;

			g_pFZN_Core->PushEvent( pEvent );
		}

		for( Wheel& rOtherWheel : m_aWheels )
		{
			if( &rOtherWheel == &rWheel )
				continue;

			if( rWheel.IsColliding( rOtherWheel, m_oCollisionPoint ) )
			{
				PhysicsEvent* pEvent = new PhysicsEvent( PhysicsEvent::Type::Collision );
				pEvent->m_oCollisionEvent.m_oCollisionPoint = m_oCollisionPoint;
				pEvent->m_oCollisionEvent.m_pRigidBodyA = &rWheel;
				pEvent->m_oCollisionEvent.m_pRigidBodyB = &rOtherWheel;

				g_pFZN_Core->PushEvent( pEvent );
			}
		}*/

		if( g_pFZN_InputMgr->IsMousePressed( sf::Mouse::Left ) )
		{
			if( pWheel->IsHovered( vMousePos ) )
			{
				m_pDraggedWheel = pWheel;
				pWheel->m_bUpdateLastPos = true;
				break;
			}
		}

		if( g_pFZN_InputMgr->IsKeyPressed( sf::Keyboard::Space ) )
		{
			pWheel->AddImpulse( { 0.f, fGravity * RandIncludeMax( 50, 100 ) * -0.01f } );
			pWheel->m_bUpdateLastPos = true;
		}

		if( g_pFZN_InputMgr->IsKeyDown( sf::Keyboard::Left ) )
		{
			pWheel->AddImpulse( { -20.f, 0.f } );
			pWheel->m_bUpdateLastPos = true;
		}
		else if( g_pFZN_InputMgr->IsKeyDown( sf::Keyboard::Right ) )
		{
			pWheel->AddImpulse( { 20.f, 0.f } );
			pWheel->m_bUpdateLastPos = true;
		}
	}

	if( g_pFZN_InputMgr->IsMousePressed( sf::Mouse::Left ) && m_pDraggedWheel == nullptr )
	{
		//m_aWheels.push_back( new Wheel() );
		//m_aWheels.back()->SetPosition( vMousePos );
		//m_daRigidBodies.push_back( m_aWheels.back() );
		m_daRigidBodies.push_back( new TestShape() );
		m_daRigidBodies.back()->SetPosition( vMousePos );
	}

	if( g_pFZN_InputMgr->IsMouseDown( sf::Mouse::Left ) && m_pDraggedWheel != nullptr )
	{
		m_pDraggedWheel->SetPosition( vMousePos );
	}
	else if( g_pFZN_InputMgr->IsMouseReleased( sf::Mouse::Left ) && m_pDraggedWheel != nullptr )
	{
		m_pDraggedWheel->m_bUpdateLastPos = true;
		m_pDraggedWheel = nullptr;
	}

	if( g_pFZN_InputMgr->IsKeyPressed( sf::Keyboard::F9 ) )
		DebugBreak();
}

void PhysicsTest::Display()
{
	/*m_oGround.Display();

	for( Wheel* pWheel : m_aWheels )
		pWheel->Display();*/

	for( RigidBody* pRigidBody : m_daRigidBodies )
		pRigidBody->Display();

	sf::CircleShape oCollision( 2.5f );
	oCollision.setFillColor( sf::Color::Yellow );
	oCollision.setOrigin( 2.5f, 2.5f );
	oCollision.setPosition( m_oCollisionPoint.m_vContactPoint );

	sf::VertexArray oCollisionResponse( sf::LineStrip, 2.f );
	oCollisionResponse[ 0 ].color = sf::Color::Yellow;
	oCollisionResponse[ 0 ].position = m_oCollisionPoint.m_vContactPoint;
	oCollisionResponse[ 1 ].color = sf::Color::Yellow;
	oCollisionResponse[ 1 ].position = m_oCollisionPoint.m_vContactPoint + m_oCollisionPoint.m_vCollisionResponse;

	g_pFZN_WindowMgr->Draw( oCollision );
	g_pFZN_WindowMgr->Draw( oCollisionResponse );

	sf::VertexArray oSimplex( sf::LinesStrip, m_daSimplex.size() );

	for( int i = 0; i < oSimplex.getVertexCount(); ++i )
	{
		oSimplex[ i ].color = sf::Color::Red;
		oSimplex[ i ].position = m_daSimplex[ i ];
	}


	g_pFZN_WindowMgr->Draw( oSimplex );
}

void PhysicsTest::_CollisionDetection()
{
	_GenerateRigidBodyPairs();

	if( m_daRigidBodyPairs.empty() )
		return;

	for( auto& rPair : m_daRigidBodyPairs )
	{
		/*if( fzn::Tools::CollisionAABBAABB( rPair.first->GetGlobalBounds(), rPair.second->GetGlobalBounds() ) == false )
			continue;*/

		if( GJKIntersection( m_daSimplex, rPair.first->m_pShape, rPair.second->m_pShape ) )
			FZN_LOG( "COLLISION !! %s", fzn::Tools::FormatedTimer( g_pFZN_Core->GetGlobalTime().asSeconds() ).c_str() );
	}

	/*if( _pRigidBody->IsColliding( *pRigidBody, m_oCollisionPoint ) )
		{
			PhysicsEvent* pEvent = new PhysicsEvent( PhysicsEvent::Type::Collision );
			pEvent->m_oCollisionEvent.m_oCollisionPoint = m_oCollisionPoint;
			pEvent->m_oCollisionEvent.m_pRigidBodyA = _pRigidBody;
			pEvent->m_oCollisionEvent.m_pRigidBodyB = pRigidBody;

			g_pFZN_Core->PushEvent( pEvent );
	}*/
}

void PhysicsTest::_GenerateRigidBodyPairs()
{
	struct RigidBodyBound
	{
		bool bBegginning{ true };
		float fScalarValue{ 0.f };
		const RigidBody* pRigidBody{ nullptr };
	};

	std::vector< RigidBodyBound > daBounds;
	daBounds.reserve( m_daRigidBodies.size() * 2.f );

	for( const RigidBody* pRigidBody : m_daRigidBodies )
	{
		sf::FloatRect oBounds = pRigidBody->GetGlobalBounds();

		if( oBounds.width <= 0.f || oBounds.height <= 0.f )
			continue;

		daBounds.push_back( { true, oBounds.top, pRigidBody } );
		daBounds.push_back( { false, oBounds.top + oBounds.height, pRigidBody } );
	}

	std::sort( daBounds.begin(), daBounds.end(), []( const RigidBodyBound& _rBoundA, const RigidBodyBound& _rBoundB ) { return _rBoundA.fScalarValue < _rBoundB.fScalarValue; } );

	m_daRigidBodyPairs.clear();
	RigidBodiesConst daActiveRigidBodies;

	for( const RigidBodyBound& rBound : daBounds )
	{
		if( rBound.bBegginning )
		{
			for( const RigidBody* pRigidBody : daActiveRigidBodies )
				m_daRigidBodyPairs.push_back( { pRigidBody, rBound.pRigidBody } );

			daActiveRigidBodies.push_back( rBound.pRigidBody );
		}
		else
			daActiveRigidBodies.erase( std::find( daActiveRigidBodies.begin(), daActiveRigidBodies.end(), rBound.pRigidBody ) );
	}
}
