#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Screen sizes
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
float SCREEN_SCALE = 1;

// Tile width and height
constexpr int WH = 16;
// Number of tile sprites
constexpr int TILE_SPRITES = 4;
// Maximum number of tiles
constexpr int MAX_TILES = 10000;
// Map width
int MAP_WIDTH = 0;
int MAP_HEIGHT = 0;

// Number of robot sprites
constexpr int ROBOT_SPRITES = 4;
// Maximum number of robots
constexpr int MAX_ROBOTS = 1000;

// Number of button sprites
constexpr int BUTTON_SPRITES = 3;
// Maximum number of buttons
constexpr int MAX_BUTTONS = 3;

// Initialise window and renderer
SDL_Window* window;
SDL_Renderer* renderer;

// Fonts
TTF_Font* pixellari = nullptr;
// Font size
constexpr int FONT_SIZE = 10;

// Texture wrapper class
class DTexture {
public:
	DTexture() {
		mTexture = nullptr;
		mWidth = 0; mHeight = 0;
	}
	~DTexture() {
		freeTexture();
	}
	void freeTexture() {
		if (mTexture != NULL) {
			SDL_DestroyTexture(mTexture);
			mTexture = NULL;
		}
	}
	bool loadTexture(std::string path, SDL_Rect mClips[] = NULL, int totalClips = 0, int wh = 0) {
		freeTexture();
		if (mTexture == nullptr) {
			SDL_Surface* loadedSurface = IMG_Load(path.c_str());
			if (loadedSurface == nullptr) {
				printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
				return false;
			}
			mTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
			if (mTexture == nullptr) {
				printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
				return false;
			}
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
			SDL_FreeSurface(loadedSurface);
		}
		if (totalClips > 0) {
			for (int i = 0; i < totalClips; i++) {
				mClips[i].x = i * wh;
				mClips[i].y = 0;
				mClips[i].w = wh;
				mClips[i].h = mHeight;
			}
		}
		return true;
	}
	bool loadText(std::string text, TTF_Font* font, SDL_Color textColor = { 255, 255, 255, 255 }) {
		freeTexture();
		SDL_Surface* loadedSurface = TTF_RenderText_Solid(font, text.c_str(), textColor);
		if (loadedSurface == nullptr) {
			printf("Unable to load text %s! SDL_TTF Error: %s\n", text.c_str(), TTF_GetError());
			return false;
		}
		mTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
		if (mTexture == nullptr) {
			printf("Unable to create texture from text surface %s! SDL Error: %s\n", text.c_str(), SDL_GetError());
			return false;
		}
		mWidth = loadedSurface->w;
		mHeight = loadedSurface->h;
		SDL_FreeSurface(loadedSurface);
		return true;
	}
	void render(float x, float y, SDL_Rect* clip = nullptr, double angle = 0.0, SDL_FPoint* center = nullptr, SDL_RendererFlip flip = SDL_FLIP_NONE, SDL_Color maskColor = { 255, 255, 255, 255 }) {
		// Screen scaling
		SDL_FRect dest = { x * SCREEN_SCALE, y * SCREEN_SCALE , (float)mWidth * SCREEN_SCALE, (float)mHeight * SCREEN_SCALE };

		// Clipping
		if (clip != nullptr) {
			dest.w = clip->w * SCREEN_SCALE;
			dest.h = clip->h * SCREEN_SCALE;
		}

		// Colour modulation
		SDL_SetTextureColorMod(mTexture, maskColor.r, maskColor.g, maskColor.b);

		// Alpha modulation
		SDL_SetTextureBlendMode(mTexture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureAlphaMod(mTexture, maskColor.a);

		// Render
		SDL_RenderCopyExF(renderer, mTexture, clip, &dest, angle, center, flip);
	}
	int getWidth() {
		return mWidth;
	}
	int getHeight() {
		return mHeight;
	}
private:
	SDL_Texture* mTexture;
	int mWidth; int mHeight;
};

// Textures
DTexture robotTexture;
DTexture tilesTexture;
DTexture textTexture;
DTexture buttonTexture;

// Texture clips
SDL_Rect robotTextureClips[ROBOT_SPRITES];
SDL_Rect tilesTextureClips[TILE_SPRITES];
SDL_Rect buttonTextureClips[BUTTON_SPRITES];

// Text
std::stringstream textObj;

// Function to render text
void renderText(std::string text, float x, float y, bool shadow = false, SDL_Color textColor = { 255, 255, 255, 255 }) {
	// Render text shadow if required
	if (shadow) {
		textTexture.loadText(text, pixellari, { 0, 0, 0, 255 });
		textTexture.render(x + (float)FONT_SIZE / 8, y + (float)FONT_SIZE / 8);
	}

	// Load and render text
	textTexture.loadText(text, pixellari, textColor);
	textTexture.render(x, y);
}

// Tile class
class Tile {
public:
	Tile(float x, float y, int setType, int setSide = -1) {
		hitbox = { x, y, WH, WH };
		type = setType;
		side = setSide;
	}
	void render(SDL_FRect& camera) {
		if (SDL_HasIntersectionF(&hitbox, &camera)) tilesTexture.render(hitbox.x - camera.x, hitbox.y - camera.y, &tilesTextureClips[type]);
	}
	SDL_FRect getBox() {
		return hitbox;
	}
	float getX() {
		return hitbox.x;
	}
	float getY() {
		return hitbox.y;
	}
	int getType() {
		return type;
	}
private:
	SDL_FRect hitbox;
	int type;
	int side; // Shelf side to retrieve item from
};

// Robot class
class Robot {
public:
	Robot(float x, float y) {
		hitbox = { x, y, WH, WH };
		battery = 100;
	}
	SDL_FRect getBox() {
		return hitbox;
	}
	void render(SDL_FRect& camera) {
		if (SDL_HasIntersectionF(&hitbox, &camera)) robotTexture.render(hitbox.x - camera.x, hitbox.y - camera.y);
	}
	// Returns the type of tile that the robot is standing on
	int getTileType(Tile* tiles[]) {
		for (int i = 0; i < MAX_TILES; i++) {
			if (tiles[i] != nullptr) {
				if (tiles[i]->getX() == hitbox.x && tiles[i]->getY() == hitbox.y) {
					return tiles[i]->getType();
				}
			}
		}
	}
	bool move(Tile* tiles[], Robot* robots[], int direction) {
		bool success = true;
		float distance = 0;

		// Move robot
		switch (direction) {
		case 0: hitbox.y -= WH; break;
		case 1: hitbox.y += WH; break;
		case 2: hitbox.x -= WH; break;
		case 3: hitbox.x += WH; break;
		}

		// Set flag to cancel robot movement if it would collide with something
		if (hitbox.x < 0 || hitbox.x > MAP_WIDTH - WH) success = false;
		else if (hitbox.y < 0 || hitbox.y > MAP_HEIGHT - WH) success = false;
		else {
			for (int i = 0; i < MAX_TILES; i++) {
				if (tiles[i] != nullptr) {
					SDL_FRect tileHitbox = tiles[i]->getBox();
					if (SDL_HasIntersectionF(&tileHitbox, &hitbox)) {
						success = false;
						break;
					}
				}
			}

			if (success) {
				for (int i = 0; i < MAX_ROBOTS; i++) {
					if (robots[i] != nullptr) {
						SDL_FRect robotHitbox = robots[i]->getBox();
						if (SDL_HasIntersectionF(&robotHitbox, &hitbox)) {
							success = false;
							break;
						}
					}
				}
			}
		}

		// Cancel robot movement
		if (!success) {
			switch (direction) {
			case 0: hitbox.y += WH; break;
			case 1: hitbox.y -= WH; break;
			case 2: hitbox.x += WH; break;
			case 3: hitbox.x -= WH; break;
			}
		}
		
		// Returns true if moved successfully
		return success;
	}
	void charge() {
		battery += 10;
		if (battery > 100) battery = 100;
	}
	void handleDecisions() {

	}
private:
	SDL_FRect hitbox;
	float battery;
};

// Button class
class Button {
public:
	Button(int x, int y, int chooseSize, float chooseScale, std::string chooseText = "") { // (x, y) here is the center of the button
		size = chooseSize;
		switch (chooseSize) {
		case 0: // Big button
			hitbox = { x, y, (int)(1000 * chooseScale), (int)(100 * chooseScale) };
			break;
		case 1: // Small button
			hitbox = { x, y, (int)(500 * chooseScale), (int)(100 * chooseScale) };
			break;
		default: // If I made a typo
			hitbox = { 0, 0, 0, 0 };
			break;
		}
		hitbox.x -= hitbox.w / 2;
		hitbox.y -= hitbox.h / 2;
		text = chooseText;
		hovered = false;
		pressed = false;
		shown = false;
		sprite = 0;
		scale = chooseScale;
		disabled = false;
	}
	void setShown(int x = -1, int y = -1) {
		shown = !shown;
		hovered = false;
		pressed = false;
		sprite = 0;

		// Set new location if specified
		if (shown && x != -1 && y != -1) {
			hitbox.x = x;
			hitbox.y = y;
			hitbox.x -= hitbox.w / 2;
			hitbox.y -= hitbox.h / 2;
		}
	}
	bool isShown() {
		return shown;
	}
	bool handleEvents(SDL_Event e) {
		bool buttonEvent = false;
		if (shown && !disabled && (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)) {
			// Get mouse position
			SDL_Point cursor = { 0, 0 };
			SDL_GetMouseState(&cursor.x, &cursor.y);

			// Hovering over button
			if (SDL_PointInRect(&cursor, &hitbox)) hovered = true;
			else hovered = false;

			if (hovered) {
				if (!pressed) sprite = 1;
				switch (e.type) {
				case SDL_MOUSEBUTTONDOWN:
					sprite = 2;
					pressed = true;
					break;
				case SDL_MOUSEBUTTONUP:
					sprite = 1;
					if (pressed) buttonEvent = true;
					pressed = false;
					break;
				}
			}
			else {
				sprite = 0;
				pressed = false;
			}
		}
		return buttonEvent;
	}
	void render() {
		SDL_Color buttonColour = { 255, 255, 255, 255 };
		if (disabled) buttonColour.a = 128;
		else buttonColour.a = 255;

		// Render button
		if (shown) {
			buttonTexture.render((float)hitbox.x, (float)hitbox.y, &buttonTextureClips[sprite + 3 * size], 0, nullptr, SDL_FLIP_NONE, buttonColour);

			if (text != "") {
				// Load text to get its dimensions
				textTexture.loadText(text, pixellari);

				// Render button text in the center of the button
				renderText(text, (float)hitbox.x + (float)hitbox.w / 2 - (float)textTexture.getWidth() / 2, (float)hitbox.y + (float)hitbox.h / 2 - (float)textTexture.getHeight() / (float)2.5, true);
			}
		}
	}
	void disable() {
		disabled = true;
	}
	void enable() {
		disabled = false;
	}
private:
	SDL_Rect hitbox;
	std::string text;
	int size;
	bool hovered;
	bool pressed;
	bool shown;
	int sprite;
	float scale;
	bool disabled;
};

// Initialises the SDL library
bool init() {
	bool success = true;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL_Init() error: %s\n", SDL_GetError());
		return false;
	}
	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0")) {
		printf("SDL_SetHint() error: %s\n", SDL_GetError());
		success = false;
	}
	if (!(window = SDL_CreateWindow("Knight", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN))) {
		printf("SDL_CreateWindow() error: %s\n", SDL_GetError());
		return false;
	}
	if (!(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC))) {
		printf("SDL_CreateRenderer() error: %s\n", SDL_GetError());
		return false;
	}
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		printf("IMG_Init() error: %s\n", SDL_GetError());
		success = false;
	}
	if (TTF_Init() == -1) {
		printf("TTF_Init() error: %s\n", SDL_GetError());
		success = false;
	}
	return success;
}

// Loads textures
bool loadAllTextures() {
	bool success = true;
	if (!tilesTexture.loadTexture("warehouse_resources/tiles.png", tilesTextureClips, TILE_SPRITES, WH)) success = false;
	if (!robotTexture.loadTexture("warehouse_resources/robot.png", robotTextureClips, ROBOT_SPRITES, WH)) success = false;
	return success;
}

// Loads fonts
bool loadFonts() {
	bool success = true;
	if (!(pixellari = TTF_OpenFont("warehouse_resources/Pixellari.ttf", 10 * (int)SCREEN_SCALE))) {
		printf("TTF_OpenFont error for Pixellari");
		success = false;
	}
	return success;
}

// Deletes objects
void close(Tile* tiles[], Robot* robots[]) {
	// Delete tiles
	for (int i = 0; i < MAX_TILES; i++) {
		if (tiles[i] != nullptr) {
			delete tiles[i];
			tiles[i] = nullptr;
		}
	}

	// Delete robots
	for (int i = 0; i < MAX_ROBOTS; i++) {
		if (robots[i] != nullptr) {
			delete robots[i];
			robots[i] = nullptr;
		}
	}
}

// Closes the SDL library
void closeSDL() {
	// Free textures
	tilesTexture.freeTexture();
	robotTexture.freeTexture();

	// Deallocate font
	pixellari = nullptr;

	// Destroy window and renderer
	SDL_DestroyRenderer(renderer);
	renderer = nullptr;
	SDL_DestroyWindow(window);
	window = nullptr;

	// Quit SDL and its sub-libraries
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

// Converts a map file into an array of tiles
bool setTiles(Tile* tiles[], std::string mapFile, int mapWidth, int mapHeight) {
	bool success = true;
	float x = 0, y = 0;
	MAP_WIDTH = mapWidth;
	MAP_HEIGHT = mapHeight;

	// Load map file
	std::ifstream map(mapFile.c_str());
	if (map.fail()) {
		printf("ifstream error: Could not load map file\n");
		success = false;
	}
	else {
		int tileType = -1;
		for (int i = 0; i < mapWidth / WH * mapHeight / WH; i++) {
			// Read tile type
			map >> tileType;
			if (map.fail()) {
				printf("Unexpected end of file after tile %d\n", i);
				success = false;
				break;
			}

			// Create tile
			if ((tileType >= 0) && (tileType <= TILE_SPRITES)) {
				tiles[i] = new Tile(x, y, tileType);
			}
			else {
				printf("Invalid tile type at %d\n", i);
				success = false;
				break;
			}

			// Go to next tile
			x += WH;

			if (x >= mapWidth) {
				// Go to next row
				x = 0;
				y += WH;
			}
		}
		map.close();
	}
	return success;
}

bool menu() {
	// Initialise buttons
	Button* buttons[MAX_BUTTONS] = { nullptr };

	return true;
}

bool simulation() {
	// Initialise objects and variables
	Tile* tiles[MAX_TILES] = { nullptr };
	Robot* robots[MAX_ROBOTS] = { nullptr };

	SDL_FRect camera = { 0, 0, (float)SCREEN_WIDTH / SCREEN_SCALE, (float)SCREEN_HEIGHT / SCREEN_SCALE };
	float camSpd = 10;
	SDL_Event e;

	// Random seed
	srand((unsigned int)time(0));

	// Initialise tiles based on map
	if (!setTiles(tiles, "warehouse_resources/test_map.map", 50 * WH, 50 * WH)) {
		printf("setTiles() error\n");
		return false;
	}

	// Initialise robots
	for (int i = 0; i < MAX_ROBOTS; i++) {
		robots[i] = new Robot(0, 0);
	}

	// Main loop
	bool quit = false;
	while (!quit) {
		// Handle events
		while (SDL_PollEvent(&e) != 0) {
			// The close button
			if (e.type == SDL_QUIT) quit = true;

			// Move camera using arrow keys
			const Uint8* keyStates = SDL_GetKeyboardState(nullptr);
			if (keyStates[SDL_SCANCODE_UP]) camera.y -= camSpd;
			if (keyStates[SDL_SCANCODE_DOWN]) camera.y += camSpd;
			if (keyStates[SDL_SCANCODE_LEFT]) camera.x -= camSpd;
			if (keyStates[SDL_SCANCODE_RIGHT]) camera.x += camSpd;
		}

		// Process robots
		for (int i = 0; i < MAX_ROBOTS; i++) {
			if (robots[i] != nullptr) {
				robots[i]->handleDecisions();
			}
		}

		// Reset screen
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(renderer);

		// Render tiles
		for (int i = 0; i < MAX_TILES; i++) {
			if (tiles[i] != nullptr) {
				tiles[i]->render(camera);
			}
		}

		// Render robots
		for (int i = 0; i < MAX_ROBOTS; i++) {
			if (robots[i] != nullptr) {
				robots[i]->render(camera);
			}
		}

		// Update the screen
		SDL_RenderPresent(renderer);
	}
	// Deallocate memory
	close(tiles, robots);

	return true;
}

int main(int argc, char** argv) {
	// Initialise SDL
	if (!init()) {
		printf("init() error\n");
		return false;
	}
	if (!loadAllTextures()) {
		printf("loadAllTextures() error\n");
		return false;
	}
	if (!loadFonts()) {
		printf("loadFonts() error\n");
		return false;
	}

	// The warehouse robot simulation
	simulation();

	// Close SDL library
	closeSDL();

	return 0;
}