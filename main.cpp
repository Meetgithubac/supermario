
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
int main()
{
    // Create the game window (200x200 size with title "SFML works!")
    sf::RenderWindow window(sf::VideoMode({ 1080, 600 }), "Super mario");       // setting game resolution.

    // Create a circle shape with radius 100 pixels
    //sf::CircleShape shape(100.f);

    // loading the music
    //sf::Music music("assets/audio/SuperMarioBros.mp3");
    //    music.play();   // playing the background music
    

    // load background image
    sf::Texture backgroundimg;
    if (!backgroundimg.loadFromFile("assets/mariobackground.png")) {
        std::cerr << "Error: Failed to load background texture!" << std::endl;
        return -1;
    }

    sf::Texture mariotexture;
    if (!mariotexture.loadFromFile("assets/mario.png")) {
        std::cerr << "Error: Failed to load mariocharcter texture!" << std::endl;
        return -1;
    }
    //mariotexture.loadFromFile("assets/mario.png");

    // making background sprite.
    sf::Sprite backgroundSprite(backgroundimg);
    backgroundSprite.setOrigin({ 100.f,100.f });

    // setting center of background.
        // 1. Get the size of the texture
          sf::Vector2u textureSize = backgroundimg.getSize();

        // 2. Set the origin of the sprite to its center
          backgroundSprite.setOrigin({ static_cast<float>(textureSize.x) / 2.f, static_cast<float>(textureSize.y) / 2.f });
        // 3. Get the center of the window
          sf::Vector2u windowSize = window.getSize();
          sf::Vector2f windowCenter(static_cast<float>(windowSize.x) / 2.f, static_cast<float>(windowSize.y) / 2.f);

        // 4. Set the position of the sprite to the center of the window
          backgroundSprite.setPosition(windowCenter);

    sf::Sprite mariosprite(mariotexture);
    mariosprite.setPosition({ 10, 10 });

    // Set the fill color of the circle to green
    //shape.setFillColor(sf::Color::Green);

    // Main game loop - runs until window is closed
    while (window.isOpen())
    {
        // Handle events (like pressing close button)
        while (auto event = window.pollEvent())
        {
            // If close event triggered
            if (event->is<sf::Event::Closed>())
                window.close();

            // Handle window resizing to keep the background centered
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                sf::FloatRect visibleArea({ 0.f, 0.f }, sf::Vector2f(resized->size));
                window.setView(sf::View(visibleArea));

                // Re-center the background on resize
                sf::Vector2u currentWindowSize = window.getSize();
                sf::Vector2f newWindowCenter(static_cast<float>(currentWindowSize.x) / 2.f, static_cast<float>(currentWindowSize.y) / 3.f);
                backgroundSprite.setPosition(newWindowCenter);
            }
        }

        // Clear the window from last frame
        window.clear();
       

        // Draw the green circle shape
        window.draw(backgroundSprite);
        window.draw(mariosprite);
        // Display what’s drawn on the screen
        window.display();
    }

    return 0;
}





