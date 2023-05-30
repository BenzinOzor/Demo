//------------------------------------------------------------------------
//Author : Philippe OFFERMANN
//Date : 29.01.15
//Description : Entry point of the program
//------------------------------------------------------------------------

#include <FZN/Includes.h>
#include <FZN/Managers/DataManager.h>
#include <FZN/Managers/WindowManager.h>

#include <SFML/Graphics/RenderWindow.hpp>

#include "demo.h"


int main()
{
	fzn::FazonCore::CreateInstance( { "Demo", FZNProjectType::Application } );
	//TEST   d
	g_pFZN_Core->GreetingMessage();
	g_pFZN_Core->ConsoleTitle( g_pFZN_Core->GetProjectName().c_str() );
	g_pFZN_Core->SetConsolePosition( sf::Vector2i( 10, 10 ) );
	g_pFZN_Core->SetDataFolder( "../../Data/" );

	g_pFZN_DataMgr->LoadResourceFile( DATAPATH( "XMLFiles/Resources" ) );			//Loading of the resources that don't belong in a resource group and filling of the map containing the paths to the resources)

	g_pFZN_WindowMgr->AddWindow( 1024, 768,  sf::Style::Default, "Checklist", sf::Vector2i( 100, 100 ) );
	g_pFZN_WindowMgr->SetWindowFramerate(60);
	
	/*g_pFZN_WindowMgr->AddWindow();
	g_pFZN_WindowMgr->SetWindowFramerate( 60, 1 );
	g_pFZN_WindowMgr->SetWindowSize( sf::Vector2u( 200, 200 ), 1 );
	g_pFZN_WindowMgr->SetWindowPosition( g_pFZN_WindowMgr->GetWindow( 0 )->getPosition() + sf::Vector2i( -200, 0 ), 1 );

	g_pFZN_WindowMgr->AddWindow();
	g_pFZN_WindowMgr->SetWindowFramerate( 120, 2 );
	g_pFZN_WindowMgr->SetWindowSize( sf::Vector2u( 200, 200 ), 2 );
	g_pFZN_WindowMgr->SetWindowPosition( g_pFZN_WindowMgr->GetWindow( 0 )->getPosition() + sf::Vector2i( -200, 230 ), 2 );
	
	g_pFZN_WindowMgr->AddWindow();
	g_pFZN_WindowMgr->SetWindowFramerate( 300, 3 );
	g_pFZN_WindowMgr->SetWindowSize( sf::Vector2u( 200, 200 ), 3 );
	g_pFZN_WindowMgr->SetWindowPosition( g_pFZN_WindowMgr->GetWindow( 0 )->getPosition() + sf::Vector2i( -200, 460 ), 3 );*/

	//Changing the icon of the sfml window (console icon is set in the file res.rc)
	g_pFZN_WindowMgr->SetIcon( DATAPATH( "Display/Pictures/fzn.png" ) );

	//Changing the titles of the window and the console
	g_pFZN_WindowMgr->SetWindowTitle( g_pFZN_Core->GetProjectName().c_str() );

	Demo fznDemo;				// Creation of the demo class
	g_pFZN_Core->GameLoop();	// Game loop (add callbacks to your functions so they can be called in there, see Tools.h / Tools.cpp)
}
