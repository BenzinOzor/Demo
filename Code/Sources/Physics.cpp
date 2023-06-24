#include <FZN/Managers/FazonCore.h>
#include <FZN/Managers/WindowManager.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Math.h>

#include <SFML/Graphics/CircleShape.hpp>

#include "Physics.h"

PhysicsTest* g_pPhysics = nullptr;

RigidBody::RigidBody()
{
}

void RigidBody::Update( float _fGravity, const std::vector< RigidBody* >& _daRigidBodies )
{
	if( m_bUpdateLastPos )
		m_vLastPos = GetPosition();

	SetPosition( GetPosition() + m_vVelocity * FrameTime );

	if( dynamic_cast< Wheel* >( this ) && m_vVelocity.y != 0.f )
	{
		FZN_LOG( "Velocity: %f", m_vVelocity.y );
	}
	m_vVelocity += sf::Vector2f( 0.f, _fGravity ) * FrameTime;

	_ComputeMovementBounds();

	if( m_bUpdateLastPos )
		fLastPosLength = fzn::Math::VectorLength( { GetPosition() - m_vLastPos } );
}

void RigidBody::Display()
{
	if( m_pShape != nullptr )
	{
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
		}
	}

	sf::RectangleShape oNextPosBounds( { m_oMouvementBounds.width, m_oMouvementBounds.height } );
	oNextPosBounds.setFillColor( { 0, 0, 0, 0 } );
	oNextPosBounds.setOutlineColor( { 255, 255, 255, 200 } );
	oNextPosBounds.setOutlineThickness( 1.f );
	oNextPosBounds.setPosition( m_oMouvementBounds.left, m_oMouvementBounds.top );

	g_pFZN_WindowMgr->Draw( oNextPosBounds );

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

void RigidBody::_OnCollision( const CollisionPoint& _rCollision )
{
	m_vVelocity += _rCollision.m_vCollisionResponse;

	if( m_bUpdateLastPos )
	{
		FZN_LOG( "COLLISION %f", _rCollision.m_vContactPoint.y );
		m_bUpdateLastPos = false;
	}
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

PhysicsTest::PhysicsTest()
{
	g_pFZN_Core->AddCallback( this, &PhysicsTest::Update, fzn::DataCallbackType::Update );
	g_pFZN_Core->AddCallback( this, &PhysicsTest::Display, fzn::DataCallbackType::Display );

	m_daRigidBodies.push_back( &m_oGround );

	g_pPhysics = this;
}

void PhysicsTest::Update()
{
	const float fGravity = 981.f;
	sf::Vector2f vMousePos = g_pFZN_WindowMgr->GetMousePosition();

	FZN_LOG( "%f", fzn::Math::VectorAngle360( { 1.f, 0.f }, { 0.f, 1.f } ) );
	FZN_LOG( "%f", fzn::Math::VectorAngle360( { 1.f, 0.f }, { 0.f, -1.f } ) );

	for( Wheel* pWheel : m_aWheels )
	{
		if( pWheel == m_pDraggedWheel )
			continue;

		_LookForCollisions( pWheel );

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
		m_aWheels.push_back( new Wheel() );
		m_aWheels.back()->SetPosition( vMousePos );
		m_daRigidBodies.push_back( m_aWheels.back() );
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
	m_oGround.Display();

	for( Wheel* pWheel : m_aWheels )
		pWheel->Display();

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

}

void PhysicsTest::_LookForCollisions( const RigidBody* _pRigidBody )
{
	for( const RigidBody* pRigidBody : m_daRigidBodies )
	{
		if( pRigidBody == _pRigidBody )
			continue;

		if( _pRigidBody->IsColliding( *pRigidBody, m_oCollisionPoint ) )
		{
			PhysicsEvent* pEvent = new PhysicsEvent( PhysicsEvent::Type::Collision );
			pEvent->m_oCollisionEvent.m_oCollisionPoint = m_oCollisionPoint;
			pEvent->m_oCollisionEvent.m_pRigidBodyA = _pRigidBody;
			pEvent->m_oCollisionEvent.m_pRigidBodyB = pRigidBody;

			g_pFZN_Core->PushEvent( pEvent );
		}
	}
}
