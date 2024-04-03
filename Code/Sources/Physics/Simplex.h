#pragma once

#include <vector>

#include <SFML/System/Vector2.hpp>


class Simplex
{
public:
	bool ContainsOrigin( sf::Vector2f& _vDIrection );

	void AddPoint( const sf::Vector2f& _vPoint );
	void RemovePoint( int _iPointIndex );

protected:
	std::vector< sf::Vector2f > m_daPoints;
};
