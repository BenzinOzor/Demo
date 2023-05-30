//------------------------------------------------------------------------
//Author : Philippe OFFERMANN
//Date : 29.01.15
//Description : Demonstration class
//------------------------------------------------------------------------

#ifndef DEMO_H
#define DEMO_H

//#include <FZN/Display/Animation.h>
#include <FZN/Display/Anm2.h>
#include <FZN/Audio/Sound.h>
#include <FZN/Audio/Music.h>
#include <FZN/Display/ProgressBar.h>
#include <Fmod/fmod.hpp>
#include <FZN/Tools/HermiteCubicSpline.h>

//hello

struct JumpTest : public sf::CircleShape
{
	JumpTest()
	{
		m_fCurrentDirection = sf::Vector2f(0.f, 0.f);
		m_fInitialDirection = sf::Vector2f(0.f, 0.f);
		m_fGravity = sf::Vector2f(0.f, 981.f);

		setRadius(50.f);
		setFillColor(sf::Color::Green);
		setPosition(200.f, 700.f);
	}
	~JumpTest()
	{
		for (int i=0 ; i<m_fPath.size(); ++i)
		{
			delete m_fPath[i];
			m_fPath[i] = nullptr;
		}
	}

	void Update();

	sf::Vector2f m_fCurrentDirection;
	sf::Vector2f m_fInitialDirection;
	sf::Vector2f m_fGravity;
	std::vector<sf::CircleShape*> m_fPath;
};

struct PhyTest
{
	void Update();
	void AddImpulse(sf::Vector2f _vImpulse);
	void AddVelocity(sf::Vector2f _vVelocity);

	sf::Vector2f m_vGravity = sf::Vector2f(0.f, 981.f);
	sf::Vector2f m_vVelocity = sf::Vector2f(0.f, 0.f);
	sf::Vector2f m_vDirection = sf::Vector2f(0.f, 0.f);
	sf::Vector2f m_vPosition = sf::Vector2f(0.f, 700.f);
	float m_fMass = 1.f;
	float m_fMaxSpeed = 0.f;
	sf::Vector2f m_vMaxValues = sf::Vector2f(0.f, 0.f);

	INT8 m_bUseHorizontalMaxValues = false;
	INT8 m_bUseVerticalMaxValues = false;
};

struct SplineTest
{
	void Init();
	void Update();
	void Display();

	void SplineTargetManagement();
	void SplineTargetDetection();
	void BuildSpline( std::vector< fzn::HermiteCubicSpline::SplineControlPoint > _oControlPoints );
	void BuildCircle();
	void CircleTargetDetection( std::vector< fzn::HermiteCubicSpline::SplineControlPoint >& _oControlPoints, float _fCheckRadius );

	fzn::HermiteCubicSpline m_oSpline;
	fzn::HermiteCubicSpline m_oSpline2;
	fzn::HermiteCubicSpline m_oSplineCenter;
	sf::VertexArray			m_oVertices;
	bool m_bUp[ 3 ];
	std::vector< sf::Vector2f > m_pSplineTargets;
	int m_iDraggedTarget;
	sf::Vector2f m_vTargetCheckerPos;
	float m_fCheckRadius;
};

struct CollisionTest
{
	struct Target
	{
		sf::Vector2f	m_vPosition;
		bool			m_bInBox = false;
	};

	void Init();
	void Update();
	void Display();

	std::vector< sf::Vector2f > m_saOrientedBoundingBox;
	std::vector< Target > m_daTargets;
	int m_iDraggedTarget;
};

class Demo
{
public:
	/////////////////CONSTRUCTOR / DESTRUCTOR/////////////////

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Default constructor
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	Demo();
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Destructor
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	~Demo();


	/////////////////OTHER FUNCTIONS/////////////////

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Update of the demonstration
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void Update();
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Display of the demonstration
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void Display();

	/////////////////MEMBER VARIABLES/////////////////

	fzn::Animation bnz;			//Demonstration animation
	fzn::Animation wumpa;		//Another demonstration animation
	fzn::Animation walkRight;
	sf::Sound sound;				//Demonstration sound
	sf::Music* music;
	sf::Text text;					//Demonstration text
	sf::Text fToFlip;				//Instruction text
	sf::Sprite sprite;				//Demonstration sprite

	fzn::ProgressBar progressBar;

	fzn::Sound testRapBot[4];
	int iCurrentSound;

	JumpTest m_jumpTest;
	PhyTest m_phyTest;
	SplineTest m_oSplineTest;
	CollisionTest m_oCollisionTest;
	
	fzn::Anm2 isek;

	sf::RenderTexture m_oTechnology;

	FMOD::System* m_system;

	//Technology m_oTechnology;
};

#endif