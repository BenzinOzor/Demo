#pragma once
#include <vector>

namespace sf
{
	class Shape;
};

class RigidBody
{
	friend class PhysicsTest;
public:
	struct CollisionPoint
	{
		sf::Vector2f	m_vContactPoint{ 0.f, 0.f };
		sf::Vector2f	m_vCollisionResponse{ 0.f, 0.f };
	};

	RigidBody();

	virtual void Update( float _fGravity, const std::vector< RigidBody* >& _daRigidBodies );
	virtual void Display();
	virtual void OnEvent();

	virtual bool IsHovered( const sf::Vector2f& _vMousePos );

	bool IsColliding( const RigidBody& _rRigiBody, CollisionPoint& _rCollisionPoint ) const;
	bool IsColliding_SAT( const RigidBody& _rRigiBody, CollisionPoint& _rCollisionPoint ) const;
	virtual void AddImpulse( const sf::Vector2f& _vImpulse );

	void SetPosition( const sf::Vector2f& _vPosition );
	sf::Vector2f GetPosition() const;
	bool HasVelocity() const;
	sf::FloatRect GetGlobalBounds() const;

	template< typename T >
	T GetShape() const
	{
		return dynamic_cast< T >( m_pShape );
	}

protected:
	virtual void _OnCollision( const CollisionPoint& _rCollision );

	void _ComputeVelocity( float _fGravity );
	void _ComputeNormals();
	void _ComputeMovementBounds();

	sf::Shape* m_pShape{ nullptr };
	std::vector< sf::Vector2f > m_aNormals;
	sf::Vector2f m_vVelocity{ 0.f, 0.f };
	sf::Vector2f m_vLastImpulse{ 0.f, 0.f };
	sf::Vector2f m_vLastPos{ 0.f, 0.f };
	float fLastPosLength{ 0.f };
	bool m_bUpdateLastPos{ true };
	sf::FloatRect m_oMouvementBounds;
};

class Wheel : public RigidBody
{
	friend class PhysicsTest;
public:
	Wheel();
	Wheel( const Wheel& _rWheel );
	~Wheel();

	virtual void Update( float _fGravity, const std::vector< RigidBody* >& _daRigidBodies ) override;
	virtual void Display() override;
	virtual bool IsHovered( const sf::Vector2f& _vMousePos ) override;

protected:
	sf::CircleShape m_oLastPosCircle;
};

class Ground : public RigidBody
{
public:
	Ground();

protected:
	virtual void _OnCollision( const CollisionPoint& _rCollision ) override {}
};

class TestShape : public RigidBody
{
public:
	TestShape();

protected:
	virtual void _OnCollision( const CollisionPoint& _rCollision ) override {}
};

class PhysicsTest
{
public:
	PhysicsTest();

	void Update();
	void Display();

	const Ground& GetGround() const { return m_oGround; }

protected:
	using RigidBodyPair		= std::pair< const RigidBody*, const RigidBody* >;
	using RigidBodyPairs	= std::vector< RigidBodyPair >;
	using RigidBodies		= std::vector< RigidBody* >;
	using RigidBodiesConst	= std::vector< const RigidBody* >;

	void _CollisionDetection();
	void _GenerateRigidBodyPairs();

	Ground m_oGround;
	std::vector< Wheel* > m_aWheels;

	RigidBodies		m_daRigidBodies;
	RigidBodyPairs	m_daRigidBodyPairs;

	RigidBody::CollisionPoint m_oCollisionPoint;
	RigidBody* m_pDraggedWheel{ nullptr };

	std::vector< sf::Vector2f > m_daSimplex;
};

extern PhysicsTest* g_pPhysics;

class PhysicsEvent
{
public:
	enum class Type
	{
		Collision,					// Global mini game event signaling a game is finished.
		COUNT
	};

	struct Collision
	{
		RigidBody::CollisionPoint m_oCollisionPoint;
		const RigidBody* m_pRigidBodyA{ nullptr };
		const RigidBody* m_pRigidBodyB{ nullptr };
	};

	PhysicsEvent() {}

	PhysicsEvent( const Type& _eType )
		: m_eType( _eType )
	{}

	Type m_eType{ Type::COUNT };

	union
	{
		Collision m_oCollisionEvent;
	};
};
