#include <SFML/Graphics.hpp> // Core SFML graphics module
#include <SFML/Graphics/Font.hpp>
#include <SFML/Window.hpp>   // SFML windowing system (for events, window management)
#include <SFML/System.hpp>   // SFML system module (for time, vectors, etc.)
#include <iostream>          // For console output (e.g., error messages)
#include <vector>            // For dynamic arrays (e.g., storing platforms, enemies)
#include <optional>          // For modern SFML 3.0 event handling

// Define global constants for game settings
const float GRAVITY = 900.0f; // Pixels per second squared (acceleration due to gravity)
const float PLAYER_JUMP_VELOCITY = -500.0f; // Initial vertical velocity when jumping (negative for upwards)
const float PLAYER_MOVE_SPEED = 200.0f;     // Player horizontal movement speed
const float ENEMY_MOVE_SPEED = 80.0f;      // Enemy horizontal movement speed
const float COLLISION_EPSILON = 0.1f;    // A small value to push objects out of collision to prevent sticking

// ====================================================================================================
// Utility Functions and Enums
// ====================================================================================================

/**
 * @brief Enum for different game states.
 * Used to control the flow and logic of the game (e.g., show main menu, play, show game over screen).
 */
enum GameState {
    Playing,    // The main game loop where gameplay occurs
    GameOver    // Game over screen, player can restart or exit
};

/**
 * @brief Checks for AABB (Axis-Aligned Bounding Box) collision between two SFML rectangles.
 * This is a common and efficient way to detect if two rectangular objects are overlapping.
 *
 * @param rect1 The first sf::FloatRect.
 * @param rect2 The second sf::FloatRect.
 * @return True if the rectangles are overlapping, false otherwise.
 */
bool checkCollision(const sf::FloatRect& rect1, const sf::FloatRect& rect2) {
    return rect1.findIntersection(rect2).has_value();
}

// ====================================================================================================
// Player Class
// Represents the main player character (Mario).
// ====================================================================================================
class Player {
public:
    sf::RectangleShape sprite;      // Visual representation of the player
    sf::Vector2f velocity;          // Current speed and direction of the player
    bool onGround;                  // Flag to check if the player is currently on a platform/ground
    sf::IntRect textureRect;        // Rectangle for animated spritesheet (if using one)
    sf::Texture playerTexture;      // Texture for the player sprite

    /**
     * @brief Constructor for the Player class.
     * Initializes player properties like size, color, initial position, and loads texture.
     */
    Player(float startX, float startY) : velocity(0.0f, 0.0f), onGround(false) {
        sprite.setSize({ 40.0f, 60.0f }); // Set the player's size
        sprite.setPosition({ startX, startY }); // Set initial position

        // Load player texture (replace "assets/mario.png" with your actual path)
        if (!playerTexture.loadFromFile("assets/mario.png")) {
            std::cerr << "Error: Could not load assets/mario_idle.png. Using placeholder color." << std::endl;
            sprite.setFillColor(sf::Color::Red);
        }
        else {
            sprite.setTexture(&playerTexture); // Set the texture to the sprite
            textureRect = sf::IntRect({ 0, 0 }, { 40, 60 }); // Assuming a single frame or idle frame
            sprite.setTextureRect(textureRect);
        }
    }

    /**
     * @brief Updates the player's state (position, velocity) based on input and physics.
     */
    void update(float deltaTime, const std::vector<sf::RectangleShape>& platforms) {
        // Apply gravity
        velocity.y += GRAVITY * deltaTime;

        // Move player horizontally
        sprite.move({ velocity.x * deltaTime, 0 });

        // Horizontal collision with platforms
        for (const auto& platform : platforms) {
            if (checkCollision(sprite.getGlobalBounds(), platform.getGlobalBounds())) {
                // If collision occurs, move player out of the platform
                if (velocity.x > 0) { // Moving right, hit left side of platform
                    sprite.setPosition({ platform.getGlobalBounds().position.x - sprite.getGlobalBounds().size.x - COLLISION_EPSILON, sprite.getPosition().y });
                }
                else if (velocity.x < 0) { // Moving left, hit right side of platform
                    sprite.setPosition({ platform.getGlobalBounds().position.x + platform.getGlobalBounds().size.x + COLLISION_EPSILON, sprite.getPosition().y });
                }
                velocity.x = 0; // Stop horizontal movement on collision
            }
        }

        // Move player vertically
        sprite.move({ 0, velocity.y * deltaTime });

        onGround = false; // Reset onGround flag each frame

        // Vertical collision with platforms
        for (const auto& platform : platforms) {
            if (checkCollision(sprite.getGlobalBounds(), platform.getGlobalBounds())) {
                if (velocity.y > 0) { // Falling down (landed on platform)
                    // Push player up to sit exactly on top of the platform
                    sprite.setPosition({ sprite.getPosition().x, platform.getGlobalBounds().position.y - sprite.getGlobalBounds().size.y });
                    velocity.y = 0; // Stop vertical movement
                    onGround = true; // Player is on the ground
                }
                else if (velocity.y < 0) { // Jumping up (hit head on platform)
                    // Push player down from the bottom of the platform
                    sprite.setPosition({ sprite.getPosition().x, platform.getGlobalBounds().position.y + platform.getGlobalBounds().size.y + COLLISION_EPSILON });
                    velocity.y = 0; // Stop upward movement
                }
            }
        }

        // Keep player within window bounds (simple edge detection)
        if (sprite.getPosition().x < 0) {
            sprite.setPosition({ 0, sprite.getPosition().y });
            velocity.x = 0;
        }
        if (sprite.getPosition().x + sprite.getGlobalBounds().size.x > 800) { // Assuming window width 800
            sprite.setPosition({ 800 - sprite.getGlobalBounds().size.x, sprite.getPosition().y });
            velocity.x = 0;
        }

        // If player falls off the bottom of the screen (game over condition)
        // This is typically handled in the main Game class to trigger Game Over state
    }

    /**
     * @brief Handles player input (keyboard presses) to control movement.
     */
    void handleInput() {
        velocity.x = 0; // Reset horizontal velocity each frame

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            velocity.x = -PLAYER_MOVE_SPEED;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            velocity.x = PLAYER_MOVE_SPEED;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && onGround) {
            velocity.y = PLAYER_JUMP_VELOCITY; // Apply upward velocity for jump
            onGround = false; // Player is no longer on the ground
        }
    }

    /**
     * @brief Draws the player sprite to the render target.
     */
    void draw(sf::RenderTarget& target) const {
        target.draw(sprite);
    }
};

// ====================================================================================================
// Enemy Class
// Represents a simple enemy (e.g., Goomba) that moves horizontally.
// ====================================================================================================
class Enemy {
public:
    sf::RectangleShape sprite;     // Visual representation of the enemy
    sf::Vector2f velocity;         // Current speed and direction of the enemy
    float leftBound;               // Leftmost point the enemy will patrol to
    float rightBound;              // Rightmost point the enemy will patrol to
    sf::Texture enemyTexture;      // Texture for the enemy sprite

    /**
     * @brief Constructor for the Enemy class.
     * Initializes enemy properties like size, color, initial position, and patrol bounds.
     */
    Enemy(float startX, float startY, float bound1, float bound2) : velocity(ENEMY_MOVE_SPEED, 0.0f) {
        sprite.setSize({ 40.0f, 40.0f }); // Set enemy size
        sprite.setPosition({ startX, startY }); // Set initial position

        // Load enemy texture (replace "assets/goomba.png" with your actual path)
        if (!enemyTexture.loadFromFile("assets/goomba.png")) {
            std::cerr << "Error: Could not load assets/goomba.png. Using placeholder color." << std::endl;
            sprite.setFillColor(sf::Color::Green);
        }
        else {
            sprite.setTexture(&enemyTexture);
        }

        leftBound = std::min(bound1, bound2); // Determine true left bound
        rightBound = std::max(bound1, bound2); // Determine true right bound
    }

    /**
     * @brief Updates the enemy's state (position).
     * The enemy moves horizontally between its defined bounds.
     */
    void update(float deltaTime) {
        sprite.move({ velocity.x * deltaTime, 0 }); // Move horizontally

        // Reverse direction if hitting patrol bounds
        if (sprite.getPosition().x <= leftBound || sprite.getPosition().x + sprite.getGlobalBounds().size.x >= rightBound) {
            velocity.x *= -1; // Reverse horizontal velocity
            // Ensure enemy is exactly at the boundary to prevent drifting
            if (sprite.getPosition().x <= leftBound) {
                sprite.setPosition({ leftBound, sprite.getPosition().y });
            }
            else { // sprite.getPosition().x + sprite.getGlobalBounds().size.x >= rightBound
                sprite.setPosition({ rightBound - sprite.getGlobalBounds().size.x, sprite.getPosition().y });
            }
        }
    }

    /**
     * @brief Draws the enemy sprite to the render target.
     */
    void draw(sf::RenderTarget& target) const {
        target.draw(sprite);
    }
};

// ====================================================================================================
// Coin Class
// Represents a collectible coin in the game.
// ====================================================================================================
class Coin {
public:
    sf::RectangleShape sprite;    // Visual representation of the coin
    bool collected;               // Flag indicating if the coin has been collected

    sf::Texture coinTexture;      // Texture for the coin sprite
    sf::Clock animationClock;     // Clock to control animation speed
    sf::IntRect textureRect;      // Current rectangle for animation frame
    int currentFrame;             // Current animation frame index
    float frameTime;              // Time duration for each animation frame
    int numFrames;                // Total number of frames in the animation

    /**
     * @brief Constructor for the Coin class.
     * Initializes coin properties, position, and animation data.
     */
    Coin(float x, float y) : collected(false), currentFrame(0), frameTime(0.1f), numFrames(4) { // Assuming 4 frames for coin animation
        sprite.setSize({ 30.0f, 30.0f }); // Set coin size
        sprite.setPosition({ x, y });       // Set position

        // Load coin texture (replace "assets/coin.png" with your actual path)
        if (!coinTexture.loadFromFile("assets/coin.png")) { // Use a single image for now
            std::cerr << "Error: Could not load assets/coin.png. Using placeholder color." << std::endl;
            sprite.setFillColor(sf::Color::Yellow);
        }
        else {
            sprite.setTexture(&coinTexture);
        }
    }

    /**
     * @brief Updates the coin's animation.
     */
    void update(float deltaTime) {
        // Simple animation logic (if using a spritesheet)
        if (coinTexture.getSize().x > sprite.getSize().x && numFrames > 1) { // Check if it's potentially a spritesheet
            if (animationClock.getElapsedTime().asSeconds() >= frameTime) {
                currentFrame = (currentFrame + 1) % numFrames;
                textureRect.position.x = currentFrame * static_cast<int>(sprite.getSize().x);
                sprite.setTextureRect(textureRect);
                animationClock.restart();
            }
        }
    }

    /**
     * @brief Draws the coin sprite to the render target if it hasn't been collected.
     */
    void draw(sf::RenderTarget& target) const {
        if (!collected) {
            target.draw(sprite);
        }
    }
};

// ====================================================================================================
// Game Class
// The main class that manages the game loop, states, entities, and rendering.
// ====================================================================================================


class Game {
public:
    sf::RenderWindow window;             // The main game window
    sf::View view;                       // The camera view, used for scrolling
    GameState currentState;              // Current state of the game
    Player player;                       // The player object
    std::vector<sf::RectangleShape> platforms; // Vector to hold all platform objects
    std::vector<Enemy> enemies;          // Vector to hold all enemy objects
    std::vector<Coin> coins;             // Vector to hold all coin objects

    sf::Texture backgroundTexture;       // Texture for the background image
    std::optional<sf::Sprite> backgroundSprite;         // Sprite for the background image


    sf::Font font;                       // Font for displaying text (e.g., score, game over)
    std::optional<sf::Text> scoreText;                  // Text object for displaying the score
    std::optional<sf::Text> gameOverText;               // Text object for "Game Over" message
    int score;                           // Player's current score
    int lives;                           // Player's lives (optional, but good for Mario-like game)

    sf::Clock clock;                     // Clock to measure elapsed time between frames

    /**
     * @brief Constructor for the Game class.
     * Initializes the game window, loads resources, sets up game entities, and sets initial state.
     */
    Game() : window(sf::VideoMode({ 800, 600 }), "Super Mario SFML"),
        currentState(Playing),
        player(100.0f, 400.0f), // Initial player position
        score(0),
        lives(3) // Initial lives
    {
        // Set up the window
        window.setFramerateLimit(60); // Limit frame rate for smoother gameplay

        // Initialize the view (camera)
        view.setCenter({ static_cast<float>(window.getSize().x) / 2.f, static_cast<float>(window.getSize().y) / 2.f });
        view.setSize({ static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y) });
        window.setView(view);

        // Load background texture and set it up for centering
        if (!backgroundTexture.loadFromFile("assets/mariobackground.png")) {
            std::cerr << "Error: Could not load assets/background.png. Background will be empty." << std::endl;
        }
        else {
            //sf::Sprite backgroundSprite(backgroundTexture);         // Sprite for the background image
            backgroundSprite = sf::Sprite(backgroundTexture);
            backgroundSprite->setTexture(backgroundTexture);
            // Center the background sprite's origin
            sf::Vector2u texSize = backgroundTexture.getSize();
            backgroundSprite->setOrigin({ static_cast<float>(texSize.x) / 2.f, static_cast<float>(texSize.y) / 2.f });
            // Position the background sprite at the center of the view initially
            backgroundSprite->setPosition(view.getCenter());
        }

        // Load font for UI text
        if (!font.openFromFile("assets/arial.ttf")) { // You might need to provide an Arial font or another .ttf file
            std::cerr << "Error: Could not load assets/arial.ttf. Text will not be displayed." << std::endl;
        }
        sf::Text scoreText(font);                  // Text object for displaying the score
        sf::Text gameOverText(font);;               // Text object for "Game Over" message
        // Set up score text
        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition({ 10, 10 }); // Position at top-left of the window

        // Set up game over text
        gameOverText.setFont(font);
        gameOverText.setCharacterSize(48);
        gameOverText.setFillColor(sf::Color::Red);
        gameOverText.setString("GAME OVER!\nPress R to Restart");
        // Center the game over text
        sf::FloatRect textBounds = gameOverText.getLocalBounds();
        gameOverText.setOrigin({ textBounds.size.x / 2.f, textBounds.size.y / 2.f });
        gameOverText.setPosition({ static_cast<float>(window.getSize().x) / 2.f, static_cast<float>(window.getSize().y) / 2.f });

        // Initialize game level elements (platforms, enemies, coins)
        initLevel();
    }

    /**
     * @brief Initializes the game level with platforms, enemies, and coins.
     * This function sets up the layout of the game world.
     */
    void initLevel() {
        platforms.clear(); // Clear existing platforms
        enemies.clear();   // Clear existing enemies
        coins.clear();     // Clear existing coins

        score = 0; // Reset score
        lives = 3; // Reset lives

        // Create ground platform
        sf::RectangleShape ground({ 2000.0f, 50.0f }); // Longer ground
        ground.setPosition({ 0, 500.0f });
        ground.setFillColor(sf::Color::Black);
        platforms.push_back(ground);

        // Create some additional platforms
        sf::RectangleShape platform1({ 200.0f, 30.0f });
        platform1.setPosition({ 250.0f, 400.0f });
        platform1.setFillColor(sf::Color(100, 50, 0)); // Brown
        platforms.push_back(platform1);

        sf::RectangleShape platform2({ 150.0f, 30.0f });
        platform2.setPosition({ 500.0f, 300.0f });
        platform2.setFillColor(sf::Color(100, 50, 0)); // Brown
        platforms.push_back(platform2);

        sf::RectangleShape platform3({ 300.0f, 30.0f });
        platform3.setPosition({ 800.0f, 450.0f });
        platform3.setFillColor(sf::Color(100, 50, 0)); // Brown
        platforms.push_back(platform3);

        sf::RectangleShape platform4({ 100.0f, 30.0f });
        platform4.setPosition({ 1100.0f, 350.0f });
        platform4.setFillColor(sf::Color(100, 50, 0)); // Brown
        platforms.push_back(platform4);

        // Add enemies
        enemies.emplace_back(300.0f, 460.0f, 250.0f, 450.0f); // Enemy on ground
        enemies.emplace_back(600.0f, 260.0f, 550.0f, 700.0f); // Enemy on platform2
        enemies.emplace_back(900.0f, 410.0f, 850.0f, 1000.0f); // Enemy on platform3

        // Add coins
        coins.emplace_back(270.0f, 360.0f); // Coin above platform1
        coins.emplace_back(550.0f, 260.0f); // Coin above platform2
        coins.emplace_back(600.0f, 260.0f);
        coins.emplace_back(850.0f, 410.0f); // Coin above platform3
        coins.emplace_back(1120.0f, 310.0f); // Coin above platform4

        // Reset player position for new game/level
        player.sprite.setPosition({ 100.0f, 400.0f });
        player.velocity = { 0.0f, 0.0f };
        player.onGround = false;
        currentState = Playing;
    }

    /**
     * @brief Runs the main game loop.
     * This function continuously processes events, updates game logic, and renders the scene.
     */
    void run() {
        while (window.isOpen()) {
            float deltaTime = clock.restart().asSeconds(); // Get time elapsed since last frame

            handleEvents(); // Process all pending SFML events

            if (currentState == Playing) {
                update(deltaTime); // Update game logic if in Playing state
            }

            render(); // Render the entire scene
        }
    }

private:
    /**
     * @brief Processes all SFML events (keyboard, mouse, window close, resize, etc.).
     * This method uses the modern SFML 3.0 event handling.
     */
    void handleEvents() {
        // Poll for events in the window
        while (std::optional<sf::Event> event = window.pollEvent()) {
            // Check if the event is a 'Closed' event (e.g., user clicks the 'x' button)
            if (event->is<sf::Event::Closed>()) {
                window.close(); // Close the window
            }

            // Handle window resizing to keep the view correctly scaled and centered
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                // Create a new view with the resized dimensions
                sf::Vector2f newSize(static_cast<float>(resized->size.x), static_cast<float>(resized->size.y));
                view.setSize(newSize);
                view.setCenter({ newSize.x / 2.f, newSize.y / 2.f }); // Reset the view to the new visible area
                window.setView(view);    // Apply the updated view to the window

                // Re-center Game Over text if window is resized while in Game Over state
                if (currentState == GameOver) {
                    sf::Text gameOverText(font);;               // Text object for "Game Over" message
                    gameOverText.setPosition({ static_cast<float>(window.getSize().x) / 2.f, static_cast<float>(window.getSize().y) / 2.f });
                }
            }

            // Handle key presses specifically when in Game Over state for restart
            if (currentState == GameOver && event->is<sf::Event::KeyPressed>()) {
                const auto& keyPressedEvent = event->getIf<sf::Event::KeyPressed>(); // Get the KeyPressed event data
                if (keyPressedEvent->code == sf::Keyboard::Key::R) {
                    initLevel(); // Restart the game by re-initializing the level
                }
            }
        }

        // Handle continuous player input (e.g., holding down a key)
        if (currentState == Playing) {
            player.handleInput();
        }
    }

    /**
     * @brief Updates the game logic for all entities (player, enemies, coins).
     * This method is called repeatedly in the game loop.
     */
    void update(float deltaTime) {
        player.update(deltaTime, platforms); // Update player position and handle collisions

        // Check if player fell off the screen
        if (player.sprite.getPosition().y > 700) { // Below screen boundary
            lives--;
            if (lives <= 0) {
                currentState = GameOver;
                return;
            }
            // Reset player position
            player.sprite.setPosition({ 100.0f, 400.0f });
            player.velocity = { 0.0f, 0.0f };
        }

        // Update enemies
        for (auto& enemy : enemies) {
            enemy.update(deltaTime);

            // Check for player-enemy collision
            if (checkCollision(player.sprite.getGlobalBounds(), enemy.sprite.getGlobalBounds())) {
                // If player is on top of enemy (stomping)
                if (player.velocity.y > 0 && // Player is falling
                    player.sprite.getGlobalBounds().position.y + player.sprite.getGlobalBounds().size.y < enemy.sprite.getGlobalBounds().position.y + 20) // Small threshold
                {
                    // Stomp! Defeat enemy.
                    enemy.sprite.setPosition({ -1000, -1000 }); // Move off-screen
                    player.velocity.y = PLAYER_JUMP_VELOCITY / 2.f; // Small bounce for player
                    score += 100; // Increase score
                }
                else {
                    // Player hit enemy from side or bottom, take damage
                    lives--;
                    player.sprite.setPosition({ 100.0f, 400.0f }); // Reset player position
                    player.velocity = { 0.0f, 0.0f }; // Reset player velocity
                    if (lives <= 0) {
                        currentState = GameOver; // Transition to Game Over state
                    }
                }
            }
        }

        // Update coins and check for player-coin collision
        for (auto& coin : coins) {
            coin.update(deltaTime);
            if (!coin.collected && checkCollision(player.sprite.getGlobalBounds(), coin.sprite.getGlobalBounds())) {
                coin.collected = true; // Mark coin as collected
                score += 10; // Increase score
            }
        }
        //if (!font.openFromFile("assets/arial.ttf")) { // You might need to provide an Arial font or another .ttf file
        //    std::cerr << "Error: Could not load assets/arial.ttf. Text will not be displayed." << std::endl;
        //}

        sf::Text scoreText(font);                  // Text object for displaying the score
        // Update score text
        scoreText.setString("Score: " + std::to_string(score) + "   Lives: " + std::to_string(lives));

        // Update view to follow player horizontally (simple scrolling)
        view.setCenter({ player.sprite.getPosition().x + player.sprite.getGlobalBounds().size.x / 2.f, view.getCenter().y });

        // Clamp view to level bounds
        float minViewX = static_cast<float>(window.getSize().x) / 2.f;
        float maxViewX = 2000.f - static_cast<float>(window.getSize().x) / 2.f; // Assuming level width is 2000
        if (view.getCenter().x < minViewX) {
            view.setCenter({ minViewX, view.getCenter().y });
        }
        else if (view.getCenter().x > maxViewX) {
            view.setCenter({ maxViewX, view.getCenter().y });
        }
        window.setView(view); // Apply the updated view

        /* if (!backgroundTexture.loadFromFile("assets/mariobackground.png")) {
             std::cerr << "Error: Could not load assets/background.png. Background will be empty." << std::endl;
         }*/
        sf::Sprite backgroundSprite(backgroundTexture);         // Sprite for the background image

        // Parallax background: make it move slower than the foreground
        backgroundSprite.setPosition({ view.getCenter().x * 0.7f, view.getCenter().y });
    }

    /**
     * @brief Renders all game elements to the window.
     * This method clears the screen, draws backgrounds, entities, and UI, then displays the result.
     */
    void render() {
        window.clear(sf::Color(135, 206, 235)); // Clear screen with a sky blue color
        /* if (!backgroundTexture.loadFromFile("assets/mariobackground.png")) {
             std::cerr << "Error: Could not load assets/background.png. Background will be empty." << std::endl;
         }*/
        sf::Sprite backgroundSprite(backgroundTexture);         // Sprite for the background image


        //if (!font.openFromFile("assets/arial.ttf")) { // You might need to provide an Arial font or another .ttf file
        //    std::cerr << "Error: Could not load assets/arial.ttf. Text will not be displayed." << std::endl;
        //}
        sf::Text scoreText(font);                  // Text object for displaying the score
        sf::Text gameOverText(font);;               // Text object for "Game Over" message
        // Draw background
        if (backgroundTexture.getSize().x > 0) { // Only draw if texture loaded successfully
            window.draw(backgroundSprite);
        }

        // Draw platforms
        for (const auto& platform : platforms) {
            window.draw(platform);
        }

        // Draw coins
        for (const auto& coin : coins) {
            coin.draw(window); // Coin class has its own draw logic
        }

        // Draw enemies
        for (const auto& enemy : enemies) {
            enemy.draw(window); // Enemy class has its own draw logic
        }

        // Draw player
        player.draw(window);

        // Draw UI elements (score, lives)
        // UI elements should be drawn relative to the window, not the view.
        sf::View currentView = window.getView();
        sf::View fixedView = window.getDefaultView();
        window.setView(fixedView);

        if (font.getInfo().family != "") { // Only draw if font loaded successfully
            window.draw(scoreText); // Draw score text
        }

        // Draw Game Over text if in Game Over state
        if (currentState == GameOver && font.getInfo().family != "") {
            window.draw(gameOverText);
        }

        // Revert to game view
        window.setView(currentView);

        window.display(); // Display everything rendered to the window
    }

};


/**
 * @brief Main entry point of the application.
 * Creates a Game object and starts the game loop.
 */
int main() {
    try {
        Game game; // Create an instance of the Game class
        game.run(); // Start the main game loop
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }

    return 0; // Indicate successful execution
}

