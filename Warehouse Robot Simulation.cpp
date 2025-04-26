#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Number of iterations for each combination of settings when testing
constexpr int TEST_ITERATIONS = 10;

// Screen sizes
int SCREEN_WIDTH = 1280;
int SCREEN_HEIGHT = 720;
float SCREEN_SCALE = 1.5;

// Number of items to be retrieved
constexpr int MAX_ITEMS_RETRIEVE = 100;
int NUMBER_ITEMS_RETRIEVE = MAX_ITEMS_RETRIEVE;

// Tile width and height
constexpr int WH = 16;
// Number of tile sprites
constexpr int TILE_SPRITES = 9;
// Maximum number of tiles
constexpr int MAX_TILES = 10000;
// Map width
int MAP_WIDTH = 50 * WH;
int MAP_HEIGHT = 50 * WH;
// Map number
int mapNumber = 1;
// Map name
std::string mapPath = "warehouse_resources/map1.map";

// Number of robot sprites
constexpr int ROBOT_SPRITES = 4;
// Maximum number of robots
constexpr int MAX_ROBOTS = 100;
// Number of robots
int NUMBER_ROBOTS = MAX_ROBOTS;
// Robot battery loss per tick of movement
float BATTERY_LOSS = (float)0.2;
// Robot battery gain per tick of charging
float BATTERY_GAIN = 5;
// Maximum weight of items that a robot can carry at once
constexpr int MAX_WEIGHT = 10;

// Number of button sprites
constexpr int BUTTON_SPRITES = 3;
// Maximum number of buttons
constexpr int MAX_BUTTONS = 10;

// Time control
Uint64 MAX_TICK_INTERVAL = 1000;
Uint64 TICK_INTERVAL = 0;

// Obstacle-generating cooldown in number of ticks
constexpr int OBSTACLE_CD = 100;
// Max number of obstacles at a time
constexpr int MAX_OBSTACLES = 100;
// Number of obstacles
int NUMBER_OBSTACLES = MAX_OBSTACLES;

// Initialise window and renderer
SDL_Window* window;
SDL_Renderer* renderer;

// Fonts
TTF_Font* pixellari = nullptr;
TTF_Font* pixellari_medium = nullptr;
TTF_Font* pixellari_title = nullptr;
// Font size
constexpr int FONT_SIZE = 30;

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
	void render(float x, float y, SDL_Rect* clip = nullptr, float scale = SCREEN_SCALE, double angle = 0.0, SDL_FPoint* center = nullptr, SDL_RendererFlip flip = SDL_FLIP_NONE, SDL_Color maskColor = { 255, 255, 255, 255 }) {
		// Screen scaling
		SDL_FRect dest = { x * scale, y * scale , (float)mWidth * scale, (float)mHeight * scale };

		// Clipping
		if (clip != nullptr) {
			dest.w = clip->w * scale;
			dest.h = clip->h * scale;
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
DTexture blackScreenTexture;

// Texture clips
SDL_Rect robotTextureClips[ROBOT_SPRITES];
SDL_Rect tilesTextureClips[TILE_SPRITES];
SDL_Rect buttonTextureClips[BUTTON_SPRITES];
SDL_Rect blackScreenTextureClip = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

// Text
std::stringstream textObj;

// Function to render small text
void renderText(std::string text, float x, float y, bool center = false, bool shadow = false, SDL_Color textColor = { 255, 255, 255, 255 }) {
	if (center) {
		textTexture.loadText(text, pixellari);
		x -= (float)textTexture.getWidth() / 2;
		y -= (float)textTexture.getHeight() / (float)2.5;
	}

	// Render text shadow if required
	if (shadow) {
		textTexture.loadText(text, pixellari, { 0, 0, 0, 255 });
		textTexture.render(x + (float)FONT_SIZE / 16, y + (float)FONT_SIZE / 16, nullptr, 1);
	}

	// Load and render text
	textTexture.loadText(text, pixellari, textColor);
	textTexture.render(x, y, nullptr, 1);
}

// Function to render medium text
void renderTextMedium(std::string text, float x, float y, bool center = false, bool shadow = false, SDL_Color textColor = { 255, 255, 255, 255 }) {
	if (center) {
		textTexture.loadText(text, pixellari_medium);
		x -= (float)textTexture.getWidth() / 2;
		y -= (float)textTexture.getHeight() / (float)2.5;
	}

	// Render text shadow if required
	if (shadow) {
		textTexture.loadText(text, pixellari_medium, { 0, 0, 0, 255 });
		textTexture.render(x + (float)FONT_SIZE / 16, y + (float)FONT_SIZE / 16, nullptr, 1);
	}

	// Load and render text
	textTexture.loadText(text, pixellari_medium, textColor);
	textTexture.render(x, y, nullptr, 1);
}

// Function to render titles
void renderTitle(std::string text, float x, float y, bool center = false, bool shadow = false, SDL_Color textColor = { 255, 255, 255, 255 }) {
	if (center) {
		textTexture.loadText(text, pixellari_title);
		x -= (float)textTexture.getWidth() / 2;
		y -= (float)textTexture.getHeight() / (float)2.5;
	}

	// Render text shadow if required
	if (shadow) {
		textTexture.loadText(text, pixellari_title, { 0, 0, 0, 255 });
		textTexture.render(x + (float)FONT_SIZE / 16, y + (float)FONT_SIZE / 16, nullptr, 1);
	}

	// Load and render text
	textTexture.loadText(text, pixellari_title, textColor);
	textTexture.render(x, y, nullptr, 1);
}

// Tile class
class Tile {
public:
	Tile(float x, float y, int setType = -1, int setItem = -1, int setWeight = -1) {
		hitbox = { x, y, WH, WH };
		type = setType;

		if (type >= 2 && type <= 5) {
			item = setItem;
			weight = setWeight;
		}
		else {
			item = -1;
			weight = -1;
		}
	}
	void render(SDL_FRect& camera) {
		if (type > 0) {
			if (SDL_HasIntersectionF(&hitbox, &camera)) {
				tilesTexture.render(hitbox.x - camera.x, hitbox.y - camera.y, &tilesTextureClips[type - 1]);
			}
		}
	}

	// Get functions
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
	int getItem() {
		return item;
	}
	int getWeight() {
		return weight;
	}

	// Set functions
	void setTileType(int setType) {
		type = setType;
	}
	void setItem(int setShelfItem) {
		item = setShelfItem;
	}
	void setWeight(int setShelfWeight) {
		weight = setShelfWeight;
	}
private:
	SDL_FRect hitbox;
	int type;
	int item;	// Item held by shelves; -1 if no item or not a shelf
	int weight; // The weight of the item
};

// Get weight of an item
int weightOf(int item) {
	int result = item % MAX_WEIGHT;
	if (result == 0) result = MAX_WEIGHT;
	return result;
}

// Robot class
class Robot {
public:
	Robot(float x, float y, int direction = 1, float setBattery = 100, int setItems[MAX_WEIGHT] = nullptr) {
		hitbox = { x, y, WH, WH };
		weightBar = { 0, -20, WH, 5 };
		battery = setBattery;
		sprite = 3;
		dir = 1;
		weight = 0;
		if (setItems == nullptr) {
			for (int i = 0; i < MAX_WEIGHT; i++) {
				items[i] = 0;
			}
		}
		else {
			for (int i = 0; i < MAX_WEIGHT; i++) {
				items[i] = setItems[i];
				weight += setItems[i];
			}
		}

		for (int i = 0; i < MAX_TILES; i++) {
			visitHistory[i] = 0;
		}
	}
	void render(SDL_FRect& camera) {
		if (SDL_HasIntersectionF(&hitbox, &camera)) {
			SDL_FPoint point = { hitbox.x, hitbox.y };
			switch (dir) {
			case 0:	robotTexture.render(hitbox.x - camera.x, hitbox.y - camera.y, &robotTextureClips[sprite], SCREEN_SCALE); break;
			case 1:	robotTexture.render(hitbox.x - camera.x, hitbox.y - camera.y, &robotTextureClips[sprite], SCREEN_SCALE, 180); break;
			case 2:	robotTexture.render(hitbox.x - camera.x, hitbox.y - camera.y, &robotTextureClips[sprite], SCREEN_SCALE, 270); break;
			case 3:	robotTexture.render(hitbox.x - camera.x, hitbox.y - camera.y, &robotTextureClips[sprite], SCREEN_SCALE, 90); break;
			}

			// Set weight bar size
			weightBar = { hitbox.x - camera.x, hitbox.y - 5 - camera.y, WH * (float)weight / (float)MAX_WEIGHT, 4 };
			weightBar.x *= SCREEN_SCALE;
			weightBar.y *= SCREEN_SCALE;
			weightBar.w *= SCREEN_SCALE;
			weightBar.h *= SCREEN_SCALE;
			// Render weight bar
			SDL_SetRenderDrawColor(renderer, 0x46, 0xa2, 0xFF, 0xFF);
			SDL_RenderFillRectF(renderer, &weightBar);
		}
	}

	// Get functions
	SDL_FRect getBox() {
		return hitbox;
	}
	int getTile(Tile* tiles[]) {
		for (int i = 0; i < MAX_TILES; i++) {
			if (tiles[i] != nullptr) {
				if (tiles[i]->getX() == hitbox.x && tiles[i]->getY() == hitbox.y) return i;
			}
		}
		return 0;
	}
	int getItem(int index) {
		return items[index];
	}
	int* getItems() {
		return items;
	}
	int getWeight() {
		return weight;
	}
	float getBattery() {
		return battery;
	}
	int getDir() {
		return dir;
	}
	int getHistory(int index) {
		if (index >= 0 && index <= MAP_WIDTH * MAP_HEIGHT / WH / WH) return visitHistory[index];
		else return 0;
	}

	void addItem(int item) {
		// Add this item to the robot
		if (item != 0) {
			for (int i = 0; i < MAX_WEIGHT; i++) {
				if (items[i] == 0) {
					items[i] = item;
					weight += weightOf(item);
					break;
				}
			}
		}
	}
	void clearItems() {
		for (int i = 0; i < MAX_WEIGHT; i++) items[i] = 0;
		
		weight = 0;
	}

	// Set functions
	void setXY(float x, float y) {
		hitbox.x = x;
		hitbox.y = y;
	}
	void setDir(int direction) {
		dir = direction;
	}
	void setBattery(float batteryLevel) {
		battery = batteryLevel;

		if (battery < 0) battery = 0;

		// Set sprite based on battery
		if (battery == 0) sprite = 0;
		else if (battery < 20) sprite = 1;
		else if (battery < 50) sprite = 2;
	}

	// Actions
	bool turn(int direction) {
		if (battery > 0 && dir != direction) {
			dir = direction;

			// Decrement battery
			battery -= BATTERY_LOSS;
			if (battery < 0) battery = 0;

			// Set sprite based on battery
			if (battery == 0) sprite = 0;
			else if (battery < 20) sprite = 1;
			else if (battery < 50) sprite = 2;

			return true;
		}
		else return false;
	}
	bool move(Tile* tiles[], Robot* robots[]) {
		bool success = true;

		if (battery > 0 && weight <= MAX_WEIGHT) {
			// Move robot
			switch (dir) {
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
						if (tiles[i]->getType() != 1 && (tiles[i]->getType() < 6 || tiles[i]->getType() > 8)) {
							SDL_FRect tileHitbox = tiles[i]->getBox();
							if (SDL_HasIntersectionF(&tileHitbox, &hitbox)) {
								success = false;
								break;
							}
						}
					}
				}

				int instances = 0;
				if (success) {
					for (int i = 0; i < NUMBER_ROBOTS; i++) {
						if (robots[i] != nullptr) {
							SDL_FRect robotHitbox = robots[i]->getBox();
							SDL_FRect selfHitbox = hitbox;

							if (SDL_HasIntersectionF(&robotHitbox, &selfHitbox)) {
								instances++;
							}

							// If hit itself + another robot
							if (instances > 1) {
								success = false;
								break;
							}
						}
					}
				}
			}

			// Cancel robot movement
			if (!success) {
				switch (dir) {
				case 0: hitbox.y += WH; break;
				case 1: hitbox.y -= WH; break;
				case 2: hitbox.x += WH; break;
				case 3: hitbox.x -= WH; break;
				}
			}
			// decrement battery
			else {
				battery -= BATTERY_LOSS;
				if (battery < 0) battery = 0;

				// Set sprite based on battery
				if (battery == 0) sprite = 0;
				else if (battery < 20) sprite = 1;
				else if (battery < 50) sprite = 2;
			}
		}
		else success = false;

		// Returns true if moved successfully
		return success;
	}
	bool takeShelfItem(Tile* tiles[], int predItemList[]) {
		int currentTile = getTile(tiles);
		int map_width = MAP_WIDTH / WH;
		int map_height = MAP_HEIGHT / WH;

		switch (dir) {
		case 0: // Up
			// Check within bounds
			if (currentTile - map_width > 0 && tiles[currentTile - map_width] != nullptr) {
				if (tiles[currentTile - map_width] != nullptr) {
					// Check that the tile above is a bottom-facing shelf
					if (tiles[currentTile - map_width]->getType() == 3) {
						// Check that the robot can still hold this item
						if (weight + tiles[currentTile - map_width]->getWeight() > MAX_WEIGHT) return false;

						// Remove this item from the predicted item list
						for (int i = 0; i < NUMBER_ITEMS_RETRIEVE; i++) {
							if (tiles[currentTile - map_width]->getItem() == predItemList[i]) {
								predItemList[i] = 0;
								break;
							}
						}

						// Take an item
						addItem(tiles[currentTile - map_width]->getItem());
					}
					else return false;
				}
				else return false;
			}
			break;
		case 1: // Down
			// Check within bounds
			if (currentTile + map_width < map_width * map_height && tiles[currentTile + map_width] != nullptr) {
				if (tiles[currentTile + map_width] != nullptr) {
					// Check that the tile below is a top-facing shelf
					if (tiles[currentTile + map_width]->getType() == 2) {
						// Check that the robot can still hold this item
						if (weight + tiles[currentTile + map_width]->getWeight() > MAX_WEIGHT) return false;

						// Remove this item from the predicted item list
						for (int i = 0; i < NUMBER_ITEMS_RETRIEVE; i++) {
							if (tiles[currentTile + map_width]->getItem() == predItemList[i]) {
								predItemList[i] = 0;
								break;
							}
						}

						// Take an item
						addItem(tiles[currentTile + map_width]->getItem());
					}
					else return false;
				}
				else return false;
			}
			break;
		case 2: // Left
			// Check within bounds
			if (currentTile % map_width > 0 && tiles[currentTile - 1] != nullptr) {
				if (tiles[currentTile - 1] != nullptr) {
					// Check that the tile to the left is a right-facing shelf
					if (tiles[currentTile - 1]->getType() == 5) {
						// Check that the robot can still hold this item
						if (weight + tiles[currentTile - 1]->getWeight() > MAX_WEIGHT) return false;

						// Remove this item from the predicted item list
						for (int i = 0; i < NUMBER_ITEMS_RETRIEVE; i++) {
							if (tiles[currentTile - 1]->getItem() == predItemList[i]) {
								predItemList[i] = 0;
								break;
							}
						}

						// Take an item
						addItem(tiles[currentTile - 1]->getItem());
					}
					else return false;
				}
				else return false;
			}
			break;
		case 3: // Right
			// Check within bounds
			if (currentTile % map_width < map_width - 1 && tiles[currentTile + 1] != nullptr) {
				if (tiles[currentTile + 1] != nullptr) {
					// Check that the tile to the right is a left-facing shelf
					if (tiles[currentTile + 1]->getType() == 4) {
						// Check that the robot can still hold this item
						if (weight + tiles[currentTile + 1]->getWeight() > MAX_WEIGHT) return false;

						// Remove this item from the predicted item list
						for (int i = 0; i < NUMBER_ITEMS_RETRIEVE; i++) {
							if (tiles[currentTile + 1]->getItem() == predItemList[i]) {
								predItemList[i] = 0;
								break;
							}
						}

						// Take an item
						addItem(tiles[currentTile + 1]->getItem());
					}
					else return false;
				}
				else return false;
			}
			break;
		}

		battery -= BATTERY_LOSS;
		if (battery < 0) battery = 0;

		// Set sprite based on battery
		if (battery == 0) sprite = 0;
		else if (battery < 20) sprite = 1;
		else if (battery < 50) sprite = 2;

		return true;
	}
	bool takeRobotItem(Tile* tiles[], Robot* robots[]) {
		bool success = false;

		int currentTile = getTile(tiles);
		int map_width = MAP_WIDTH / WH;
		int map_height = MAP_HEIGHT / WH;

		switch (dir) {
		case 0: // Up
			// Check that there is a robot above
			for (int i = 0; i < NUMBER_ROBOTS; i++) {
				if (robots[i] != nullptr) {
					if (robots[i]->getBox().x == hitbox.x && robots[i]->getBox().y == hitbox.y - WH) {
						// If we have nothing on hand
						if (weight == 0) {
							// Add all of the dead robot's items to our hand
							for (int j = 0; j < MAX_WEIGHT; j++) addItem(robots[i]->getItem(j));

							// Clear the dead robot's hand
							robots[i]->clearItems();
							success = true;
							break;
						}
					}
				}
			}
			break;
		case 1: // Down
			// Check that there is a robot below
			for (int i = 0; i < NUMBER_ROBOTS; i++) {
				if (robots[i] != nullptr) {
					if (robots[i]->getBox().x == hitbox.x && robots[i]->getBox().y == hitbox.y + WH) {
						// If we have nothing on hand
						if (weight == 0) {
							// Add all of the dead robot's items to our hand
							for (int j = 0; j < MAX_WEIGHT; j++) addItem(robots[i]->getItem(j));

							// Clear the dead robot's hand
							robots[i]->clearItems();
							success = true;
							break;
						}
					}
				}
			}
			break;
		case 2: // Left
			// Check that there is a robot to the left
			for (int i = 0; i < NUMBER_ROBOTS; i++) {
				if (robots[i] != nullptr) {
					if (robots[i]->getBox().x == hitbox.x - WH && robots[i]->getBox().y == hitbox.y) {
						// If we have nothing on hand
						if (weight == 0) {
							// Add all of the dead robot's items to our hand
							for (int j = 0; j < MAX_WEIGHT; j++) addItem(robots[i]->getItem(j));

							// Clear the dead robot's hand
							robots[i]->clearItems();
							success = true;
							break;
						}
					}
				}
			}
			break;
		case 3: // Right
			// Check that there is a robot to the right
			for (int i = 0; i < NUMBER_ROBOTS; i++) {
				if (robots[i] != nullptr) {
					if (robots[i]->getBox().x == hitbox.x + WH && robots[i]->getBox().y == hitbox.y) {
						// If we have nothing on hand
						if (weight == 0) {
							// Add all of the dead robot's items to our hand
							for (int j = 0; j < MAX_WEIGHT; j++) addItem(robots[i]->getItem(j));

							// Clear the dead robot's hand
							robots[i]->clearItems();
							success = true;
							break;
						}
					}
				}
			}
			break;
		}

		if (success) {
			// Decrement battery
			battery -= BATTERY_LOSS;
			if (battery < 0) battery = 0;

			// Set sprite based on battery
			if (battery == 0) sprite = 0;
			else if (battery < 20) sprite = 1;
			else if (battery < 50) sprite = 2;
		}
		
		return success;
	}
	bool charge(Tile* tiles[]) {
		// If standing on a charger tile, increase battery level
		if (tiles[getTile(tiles)] != nullptr) {
			if (tiles[getTile(tiles)]->getType() == 6) {
				battery += BATTERY_GAIN;
				if (battery > 100) battery = 100;

				// Set sprite based on battery
				if (battery == 0) sprite = 0;
				else if (battery < 20) sprite = 1;
				else if (battery < 50) sprite = 2;
				else sprite = 3;

				return true;
			}
		}
		return false;
	}
	bool passItem(Robot* robots[], Tile* tiles[], int item) {
		int currentTile = getTile(tiles);
		int map_width = MAP_WIDTH / WH;
		int map_height = MAP_HEIGHT / WH;

		switch (dir) {
		case 0: // Up
			// Check within bounds
			if (currentTile - map_width > 0 && tiles[currentTile - map_width] != nullptr) {
				// Check that there is a robot above
				for (int i = 0; i < NUMBER_ROBOTS; i++) {
					if (robots[i] != nullptr) {
						// If this is the right robot
						if (robots[i]->getBox().x == hitbox.x && robots[i]->getBox().y == hitbox.y - WH) {
							// If that robot has room for the item
							if (robots[i]->getWeight() <= MAX_WEIGHT - weightOf(item)) {
								// Remove item from this robot
								for (int j = 0; j < MAX_WEIGHT; j++) {
									if (items[j] == item) {
										items[j] = 0;
										weight -= weightOf(item);
										break;
									}
								}

								// Pass the item to that robot
								robots[i]->addItem(item);

								battery -= BATTERY_LOSS;
								if (battery < 0) battery = 0;

								// Set sprite based on battery
								if (battery == 0) sprite = 0;
								else if (battery < 20) sprite = 1;
								else if (battery < 50) sprite = 2;

								return true;
							}
							break;
						}
					}
				}
			}
			break;
		case 1: // Down
			// Check within bounds
			if (currentTile + map_width < map_width * map_height && tiles[currentTile + map_width] != nullptr) {
				// Check that there is a robot below
				for (int i = 0; i < NUMBER_ROBOTS; i++) {
					if (robots[i] != nullptr) {
						// If this is the right robot
						if (robots[i]->getBox().x == hitbox.x && robots[i]->getBox().y == hitbox.y + WH) {
							// If that robot has room for the item
							if (robots[i]->getWeight() <= MAX_WEIGHT - item) {
								// Remove item from this robot
								for (int j = 0; j < MAX_WEIGHT; j++) {
									if (items[j] == item) {
										items[j] = 0;
										weight -= item;
										break;
									}
								}

								// Pass the item to that robot
								robots[i]->addItem(item);

								battery -= BATTERY_LOSS;
								if (battery < 0) battery = 0;

								// Set sprite based on battery
								if (battery == 0) sprite = 0;
								else if (battery < 20) sprite = 1;
								else if (battery < 50) sprite = 2;

								return true;
							}
							break;
						}
					}
				}
			}
			break;
		case 2: // Left
			if (currentTile % map_width > 0 && tiles[currentTile - 1] != nullptr) {
				// Check that there is a robot to the left
				for (int i = 0; i < NUMBER_ROBOTS; i++) {
					if (robots[i] != nullptr) {
						// If this is the right robot
						if (robots[i]->getBox().x == hitbox.x - WH && robots[i]->getBox().y == hitbox.y) {
							// If that robot has room for the item
							if (robots[i]->getWeight() <= MAX_WEIGHT - item) {
								// Remove item from this robot
								for (int j = 0; j < MAX_WEIGHT; j++) {
									if (items[j] == item) {
										items[j] = 0;
										weight -= item;
										break;
									}
								}

								// Pass the item to that robot
								robots[i]->addItem(item);

								battery -= BATTERY_LOSS;
								if (battery < 0) battery = 0;

								// Set sprite based on battery
								if (battery == 0) sprite = 0;
								else if (battery < 20) sprite = 1;
								else if (battery < 50) sprite = 2;

								return true;
							}
							break;
						}
					}
				}
			}
			break;
		case 3: // Right
			if (currentTile % map_width < map_width - 1 && tiles[currentTile + 1] != nullptr) {
				// Check that there is a robot to the left
				for (int i = 0; i < NUMBER_ROBOTS; i++) {
					if (robots[i] != nullptr) {
						// If this is the right robot
						if (robots[i]->getBox().x == hitbox.x + WH && robots[i]->getBox().y == hitbox.y) {
							// If that robot has room for the item
							if (robots[i]->getWeight() <= MAX_WEIGHT - item) {
								// Remove item from this robot
								for (int j = 0; j < MAX_WEIGHT; j++) {
									if (items[j] == item) {
										items[j] = 0;
										weight -= item;
										break;
									}
								}

								// Pass the item to that robot
								robots[i]->addItem(item);

								battery -= BATTERY_LOSS;
								if (battery < 0) battery = 0;

								// Set sprite based on battery
								if (battery == 0) sprite = 0;
								else if (battery < 20) sprite = 1;
								else if (battery < 50) sprite = 2;

								return true;
							}
							break;
						}
					}
				}
			}
			break;
		}

		return false;
	}
	bool submitItems(Tile* tiles[], int itemList[]) {
		bool success = false;
		// If standing on a submission tile
		if (tiles[getTile(tiles)] != nullptr) {
			if (tiles[getTile(tiles)]->getType() == 8) {
				for (int j = 0; j < MAX_WEIGHT; j++) {
					if (items[j] != 0) {
						for (int i = 0; i < NUMBER_ITEMS_RETRIEVE; i++) {
							if (itemList[i] != 0 && items[j] == itemList[i]) {
								// Submit the item
								weight -= weightOf(items[j]);
								items[j] = 0;
								itemList[i] = 0;

								success = true;
							}
						}
					}
				}
			}
		}
		if (success) {
			// Decrement battery
			battery -= BATTERY_LOSS;
			if (battery < 0) battery = 0;

			// Set sprite based on battery
			if (battery == 0) sprite = 0;
			else if (battery < 20) sprite = 1;
			else if (battery < 50) sprite = 2;
		}

		return success;
	}

	void sight(Tile* tiles[], Tile* tileDatabase[]) {
		int currentTile = getTile(tiles);
		int sightRange = 10;
		int map_width = MAP_WIDTH / WH;
		int map_height = MAP_HEIGHT / WH;

		bool stop = false;
		for (int i = 0; i < sightRange && !stop; i++) {
			if (tiles[currentTile] != nullptr) {
				// Record tile in database
				if (tileDatabase[currentTile] != nullptr) {
					tileDatabase[currentTile]->setTileType(tiles[currentTile]->getType());
					tileDatabase[currentTile]->setItem(tiles[currentTile]->getItem());
					tileDatabase[currentTile]->setWeight(tiles[currentTile]->getWeight());
				}

				// Stop sight if this tile is a shelf or wall or obstacle
				if (tiles[currentTile]->getType() >= 2 && tiles[currentTile]->getType() <= 5 || tiles[currentTile]->getType() == 0 || tiles[currentTile]->getType() == 9) break;

				// Stop sight if next tile is out of bounds
				switch (dir) {
				case 0: // Up
					if (currentTile - map_width < 0) stop = true;
					else if (tiles[currentTile - map_width] == nullptr) stop = true;

					// Go to next tile
					else currentTile -= map_width;

					break;
				case 1: // Down
					if (currentTile + map_width >= map_width * map_height) stop = true;
					else if (tiles[currentTile + map_width] == nullptr) stop = true;

					// Go to next tile
					else currentTile += map_width;

					break;
				case 2: // Left
					if (currentTile % map_width == 0) stop = true;
					else if (tiles[currentTile - 1] == nullptr) stop = true;

					// Go to next tile
					else currentTile--;

					break;
				case 3: // Right
					if (currentTile % map_width == map_width - 1) stop = true;
					else if (tiles[currentTile + 1] == nullptr) stop = true;

					// Go to next tile
					else currentTile++;

					break;
				}
			}
		}
	}
	void updateHistory(Tile* tiles[]) {
		int currentTile = getTile(tiles);

		if (tiles[currentTile] != nullptr) visitHistory[currentTile]++;
	}
	void resetHistory() {
		for (int i = 0; i < MAX_TILES; i++) {
			visitHistory[i] = 0;
		}
	}
private:
	SDL_FRect hitbox;
	SDL_FRect weightBar;
	float battery;
	int sprite;
	int dir; // 0: up, 1: down, 2: left, 3: right
	int items[MAX_WEIGHT]; // the items being held by the robot
	int weight; // the current weight of items that the robot is carrying
	int visitHistory[MAX_TILES]; // How many times it has visited this tile
};

// Button class
class Button {
public:
	Button(int x, int y, std::string chooseText = "") { // (x, y) here is the center of the button
		hitbox = { x, y, 750, 100 };
		hitbox.x -= hitbox.w / 2;
		hitbox.y -= hitbox.h / 2;
		text = chooseText;
		hovered = false;
		pressed = false;
		shown = true;
		sprite = 0;
		disabled = false;
	}
	void setShown() {
		shown = !shown;
		hovered = false;
		pressed = false;
		sprite = 0;
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
			buttonTexture.render((float)hitbox.x, (float)hitbox.y, &buttonTextureClips[sprite], 1, 0, nullptr, SDL_FLIP_NONE, buttonColour);

			if (text != "") {
				// Load text to get its dimensions
				textTexture.loadText(text, pixellari_medium);

				// Render button text in the center of the button
				renderTextMedium(text, (float)hitbox.x + (float)hitbox.w / 2, (float)hitbox.y + (float)hitbox.h / 2, true, true);
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
	bool hovered;
	bool pressed;
	bool shown;
	int sprite;
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
	SDL_DisplayMode mode;
	SDL_GetDisplayMode(0, 0, &mode);
	SCREEN_WIDTH = mode.w;
	SCREEN_HEIGHT = mode.h;
	if (!(window = SDL_CreateWindow("Warehouse Robot Simulation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN))) {
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
	if (!buttonTexture.loadTexture("warehouse_resources/button.png", buttonTextureClips, BUTTON_SPRITES, 750)) success = false;
	if (!blackScreenTexture.loadTexture("warehouse_resources/black_screen.png")) success = false;
	return success;
}

// Loads fonts
bool loadFonts() {
	bool success = true;
	if (!(pixellari = TTF_OpenFont("warehouse_resources/Pixellari.ttf", FONT_SIZE))) {
		printf("TTF_OpenFont error for Pixellari");
		success = false;
	}
	if (!(pixellari_medium = TTF_OpenFont("warehouse_resources/Pixellari.ttf", (int)(FONT_SIZE * 1.5)))) {
		printf("TTF_OpenFont error for Pixellari");
		success = false;
	}
	if (!(pixellari_title = TTF_OpenFont("warehouse_resources/Pixellari.ttf", (int)(FONT_SIZE * 2.5)))) {
		printf("TTF_OpenFont error for Pixellari");
		success = false;
	}
	return success;
}

// Closes the SDL library
void closeSDL() {
	// Free textures
	tilesTexture.freeTexture();
	robotTexture.freeTexture();
	textTexture.freeTexture();
	buttonTexture.freeTexture();

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
int setTiles(Tile* tiles[], Tile* tileDatabase[], std::string mapFile, int mapWidth, int mapHeight) {
	bool success = true;
	float x = 0, y = 0;
	MAP_WIDTH = mapWidth;
	MAP_HEIGHT = mapHeight;
	int numberOfShelves = 0;

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

			// Create tiles
			if ((tileType >= 0) && (tileType <= TILE_SPRITES)) {
				// Set the first 10 shelves to have items 1 to 10. Remaining shelves have random items.
				if (tileType >= 2 && tileType <= 5) {
					tiles[i] = new Tile(x, y, tileType, numberOfShelves + 1, weightOf(numberOfShelves + 1));
					//else tiles[i] = new Tile(x, y, tileType, rand() weightOf());

					numberOfShelves++;
				}
				else tiles[i] = new Tile(x, y, tileType);

				// Create tile in tileDatabase
				tileDatabase[i] = new Tile(x, y);
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
	if (success) return numberOfShelves;
	else return false;
}

// Just a function declaration
int simulation(bool saveResults, int iteration);

// Initialise buttons
Button* buttons[MAX_BUTTONS] = { nullptr };

// Performance metrics
int successfulRuns = 0;
int failedRuns = 0;
int ticksTaken[TEST_ITERATIONS] = { 0 };
float ticksTakenPerItem[TEST_ITERATIONS] = { (float)0 };
int numberDeadRobots[TEST_ITERATIONS] = { 0 };
float timeTaken[TEST_ITERATIONS] = { (float)0 };

void resetMetrics() {
	successfulRuns = 0;
	failedRuns = 0;

	for (int i = 0; i < TEST_ITERATIONS; i++) {
		ticksTaken[i] = 0;
		ticksTakenPerItem[i] = (float)0;
		numberDeadRobots[i] = 0;
		timeTaken[i] = (float)0;
	}
}

// Main menu
void menu() {
	// Initialise variables
	SDL_Event e;

	// Create buttons
	buttons[0] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, "Start");
	buttons[8] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 150, "Test All");
	buttons[1] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 2 * 150, "Settings");
	buttons[2] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 3 * 150, "Quit");

	buttons[3] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 3 * 150, "Back");
	buttons[4] = new Button(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2 - 170, "Next"); // Change map number
	buttons[5] = new Button(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2 + 50, "Change"); // Change number of robots
	buttons[6] = new Button(3 * SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2 + 50, "Change"); // Change number of obstacles
	buttons[7] = new Button(3 * SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2 - 170, "Change"); // Change tick speed
	// Hide settings buttons initially
	for (int i = 3; i <= 7; i++) {
		if (buttons[i] != nullptr) buttons[i]->setShown();
	}
	
	bool quit = false;
	bool startSimulation = false;
	bool testAll = false;
	bool startTraining = false;
	bool changeSettings = false;

	// Main loop
	while (!quit) {
		// Handle events
		while (SDL_PollEvent(&e) != 0) {
			// The close button
			if (e.type == SDL_QUIT) quit = true;

			// Start button
			if (buttons[0]->isShown() && buttons[0]->handleEvents(e)) {
				quit = true;
				startSimulation = true;
			}

			// Settings button and Back button
			if (buttons[1]->isShown() && buttons[1]->handleEvents(e) || buttons[3]->isShown() && buttons[3]->handleEvents(e)) {
				changeSettings = !changeSettings;
				for (int i = 0; i < MAX_BUTTONS; i++) {
					if (buttons[i] != nullptr) buttons[i]->setShown();
				}
			}

			// Quit button
			if (buttons[2]->isShown() && buttons[2]->handleEvents(e)) quit = true;

			// Change map number
			if (buttons[4]->isShown() && buttons[4]->handleEvents(e)) {
				mapNumber++;
				if (mapNumber > 8) mapNumber = 1;

				switch (mapNumber) {
				case 1:
					MAP_WIDTH = 50 * WH;
					MAP_HEIGHT = 50 * WH;
					break;
				case 2:
					MAP_WIDTH = 50 * WH;
					MAP_HEIGHT = 50 * WH;
					break;
				case 3:
					MAP_WIDTH = 50 * WH;
					MAP_HEIGHT = 50 * WH;
					break;
				case 4:
					MAP_WIDTH = 25 * WH;
					MAP_HEIGHT = 25 * WH;
					break;
				case 5:
					MAP_WIDTH = 25 * WH;
					MAP_HEIGHT = 25 * WH;
					break;
				case 6:
					MAP_WIDTH = 100 * WH;
					MAP_HEIGHT = 100 * WH;
					break;
				case 7:
					MAP_WIDTH = 100 * WH;
					MAP_HEIGHT = 100 * WH;
					break;
				case 8:
					MAP_WIDTH = 100 * WH;
					MAP_HEIGHT = 100 * WH;
					break;
				}

				textObj.str("");
				textObj << "warehouse_resources/map" << mapNumber << ".map";
				mapPath = textObj.str().c_str();
			}

			// Change number of robots
			if (buttons[5]->isShown() && buttons[5]->handleEvents(e)) {
				if (NUMBER_ROBOTS < 30) NUMBER_ROBOTS += 5;
				else NUMBER_ROBOTS += 10;

				if (NUMBER_ROBOTS > MAX_ROBOTS) NUMBER_ROBOTS = 1;
				else if (NUMBER_ROBOTS == 6) NUMBER_ROBOTS -= 1;
			}

			// Change number of obstacles
			if (buttons[6]->isShown() && buttons[6]->handleEvents(e)) {
				if (NUMBER_OBSTACLES < 30) NUMBER_OBSTACLES += 5;
				else NUMBER_OBSTACLES += 10;

				if (NUMBER_OBSTACLES > MAX_OBSTACLES) NUMBER_OBSTACLES = 0;
			}

			// Change tick speed
			if (buttons[7]->isShown() && buttons[7]->handleEvents(e)) {
				TICK_INTERVAL += 100;
				if (TICK_INTERVAL > MAX_TICK_INTERVAL) TICK_INTERVAL = 0;
			}

			// Test All
			if (buttons[8]->isShown() && buttons[8]->handleEvents(e)) {
				quit = true;
				testAll = true;
			}
		}

		// Reset screen
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(renderer);

		// Render text
		if (!changeSettings) renderTitle("Warehouse Robot Simulation", (float)SCREEN_WIDTH / 2, 300, true);
		else {
			renderTitle("Settings", (float)SCREEN_WIDTH / 2, 150, true);

			textObj.str("");
			textObj << "Map Number: " << mapNumber;
			renderTitle(textObj.str().c_str(), (float)SCREEN_WIDTH / 4, (float)SCREEN_HEIGHT / 2 - 280, true);

			textObj.str("");
			textObj << "Tick Interval (ms): " << TICK_INTERVAL;
			renderTitle(textObj.str().c_str(), (float)3 * SCREEN_WIDTH / 4, (float)SCREEN_HEIGHT / 2 - 280, true);

			textObj.str("");
			textObj << "Number of Robots: " << NUMBER_ROBOTS;
			renderTitle(textObj.str().c_str(), (float)SCREEN_WIDTH / 4, (float)SCREEN_HEIGHT / 2 - 60, true);

			textObj.str("");
			textObj << "Number of Obstacles: " << NUMBER_OBSTACLES;
			renderTitle(textObj.str().c_str(), (float)3 * SCREEN_WIDTH / 4, (float)SCREEN_HEIGHT / 2 - 60, true);
		}

		// Render buttons
		for (int i = 0; i < MAX_BUTTONS; i++) {
			if (buttons[i] != nullptr) {
				buttons[i]->render();
			}
		}

		// Update the screen
		SDL_RenderPresent(renderer);
	}
	// Delete buttons
	for (int i = 0; i < MAX_BUTTONS; i++) {
		if (buttons[i] != nullptr) {
			delete buttons[i];
			buttons[i] = nullptr;
		}
	}

	// Run simulation for the chosen settings
	if (startSimulation) simulation(false, 0);
	// Test all combinations of settings without rendering
	else if (testAll) {
		bool stop = false;

		// Create a file
		std::ofstream resultsFile("simulation test results.txt");

		// i is the map number
		for (int i = 1; i <= 8; i++) {
			mapNumber = i;

			switch (mapNumber) {
			case 1:
				MAP_WIDTH = 50 * WH;
				MAP_HEIGHT = 50 * WH;
				break;
			case 2:
				MAP_WIDTH = 50 * WH;
				MAP_HEIGHT = 50 * WH;
				break;
			case 3:
				MAP_WIDTH = 50 * WH;
				MAP_HEIGHT = 50 * WH;
				break;
			case 4:
				MAP_WIDTH = 25 * WH;
				MAP_HEIGHT = 25 * WH;
				break;
			case 5:
				MAP_WIDTH = 25 * WH;
				MAP_HEIGHT = 25 * WH;
				break;
			case 6:
				MAP_WIDTH = 100 * WH;
				MAP_HEIGHT = 100 * WH;
				break;
			case 7:
				MAP_WIDTH = 100 * WH;
				MAP_HEIGHT = 100 * WH;
				break;
			case 8:
				MAP_WIDTH = 100 * WH;
				MAP_HEIGHT = 100 * WH;
				break;
			}

			textObj.str("");
			textObj << "warehouse_resources/map" << mapNumber << ".map";
			mapPath = textObj.str().c_str();

			// j is the number of robots
			for (int j = 1; j <= 100 && !stop; j += 10) {
				if (j == 11) j--;
				if (j > 1 && j <= 35) j -= 5;
				NUMBER_ROBOTS = j;

				// k is the number of obstacles
				for (int k = 0; k <= 100 && !stop; k += 10) {
					if (k > 0 && k <= 35) k -= 5;
					NUMBER_OBSTACLES = k;

					// Run a predefined number of iterations for each combination of settings
					resetMetrics();
					for (int l = 0; l < TEST_ITERATIONS && !stop; l++) {
						int decide = simulation(true, l);
						if (decide == 1) {
							// Calculate average metrics
							float averageTicksTaken = (float)0;
							for (int m = 0; m < TEST_ITERATIONS; m++) averageTicksTaken += (float)ticksTaken[m];
							averageTicksTaken /= (float)successfulRuns;

							float averageTicksTakenPerItem = (float)0;
							for (int m = 0; m < TEST_ITERATIONS; m++) averageTicksTakenPerItem += ticksTakenPerItem[m];
							averageTicksTakenPerItem /= (float)successfulRuns;

							float averageDeadRobots = (float)0;
							for (int m = 0; m < TEST_ITERATIONS; m++) averageDeadRobots += (float)numberDeadRobots[m];
							averageDeadRobots /= (float)successfulRuns;

							float averageTimeTaken = (float)0;
							for (int m = 0; m < TEST_ITERATIONS; m++) averageTimeTaken += timeTaken[m];
							averageTimeTaken /= (float)successfulRuns;

							resultsFile << "(Map: " << mapNumber << ", Robots: " << NUMBER_ROBOTS << ", Obstacles: " << NUMBER_OBSTACLES << ")\n";
							resultsFile << "Successful Runs: " << successfulRuns << "\n";
							resultsFile << "Failed Runs: " << failedRuns << "\n";
							resultsFile << "Average Ticks Taken: " << averageTicksTaken << "\n";
							resultsFile << "Average Ticks Taken Per Item: " << averageTicksTakenPerItem << "\n";
							resultsFile << "Average Dead Robots: " << averageDeadRobots << "\n";
							resultsFile << "Average Time Taken (seconds): " << averageTimeTaken << "\n";
							resultsFile << "--------------------------------------------\n";
						}
						// Quit
						else if (decide == 0) stop = true;
						// Skip
						else if (decide == 2) break;
					}
				}
			}
		}

		resultsFile.close();
	}
}

// Initialise objects
Tile* tiles[MAX_TILES] = { nullptr };
Tile* tileDatabase[MAX_TILES] = { nullptr };
Robot* robots[MAX_ROBOTS] = { nullptr };

// Main simulation code
int simulation(bool saveResults, int iteration) {
	// Initialise variables
	SDL_FRect camera = { 0, 0, (float)SCREEN_WIDTH / SCREEN_SCALE, (float)SCREEN_HEIGHT / SCREEN_SCALE };
	float camSpd = 10;
	float camVelX = 0;
	float camVelY = 0;
	SDL_Event e;

	// Results screen variables
	int ticks = 0;
	int itemsRetrieved = 0;
	int numDeadRobots = 0;
	Uint64 runtime = SDL_GetTicks64();

	// Create buttons
	buttons[0] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, "Resume");
	buttons[1] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 2 * 150, "Finish");
	buttons[3] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 3 * 150, "Quit");
	buttons[2] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 150, "Menu");
	for (int i = 0; i < MAX_BUTTONS; i++) {
		if (buttons[i] != nullptr) buttons[i]->setShown();
	}

	if (saveResults) buttons[2]->disable();

	// Random seed
	srand((unsigned int)time(0));

	int itemList[MAX_ITEMS_RETRIEVE] = { 0 };
	int predItemList[MAX_ITEMS_RETRIEVE] = { 0 };
	int numberOfShelves = 0;

	// Event flag
	bool returnMenu = false;
	bool finishSimulation = false;
	bool skip = false;

	// Create tiles based on map
	numberOfShelves = setTiles(tiles, tileDatabase, mapPath, MAP_WIDTH, MAP_HEIGHT);
	if (numberOfShelves == 0) printf("setTiles() error\n");
	else {
		// in tileDatabase, set all black tiles (type 0)
		for (int i = 0; i < MAX_TILES; i++) {
			if (tiles[i] != nullptr) {
				if (tiles[i]->getType() == 0) tileDatabase[i]->setTileType(tiles[i]->getType());
			}
		}

		// Create robots in random valid locations
		for (int i = 0; i < NUMBER_ROBOTS; i++) {
			int spawnX = 0;
			int spawnY = 0;

			bool valid = false;
			while (!valid) {
				spawnX = WH * (rand() % MAP_WIDTH / WH);
				spawnY = WH * (rand() % MAP_HEIGHT / WH);

				for (int j = 0; j < MAX_TILES; j++) {
					if (tiles[j] != nullptr) {
						if (tiles[j]->getX() == spawnX && tiles[j]->getY() == spawnY) {
							if (tiles[j]->getType() == 1 || tiles[j]->getType() >= 6 && tiles[j]->getType() <= 8) {
								valid = true;
								break;
							}
						}
					}
				}
			}
			robots[i] = new Robot((float)spawnX, (float)spawnY);
		}

		// List of items to retrieve
		for (int i = 0; i < NUMBER_ITEMS_RETRIEVE; i++) {
			itemList[i] = rand() % numberOfShelves + 1;
			predItemList[i] = itemList[i];
		}

		// Time control
		Uint64 lastTick = 0;

		bool receivingItem[MAX_ROBOTS] = { false };
		bool chargerKnown = false;
		int deadRobot[MAX_ROBOTS] = { -1 }; // index is the dead robot, value is the robot to rescue it
		float currentGoal[MAX_ROBOTS] = { -1 }; // 0: explore, 1: shelf, 2: charger, 3: exit, 4: dead robot

		// Print settings
		printf("Running simulation %d for:\n", iteration + 1);
		printf("> Map %d\n", mapNumber);
		if (NUMBER_ROBOTS == 1) printf("> %d robot\n", NUMBER_ROBOTS);
		else printf("> %d robots\n", NUMBER_ROBOTS);
		printf("> %d obstacles\n\n", NUMBER_OBSTACLES);

		bool quit = false;
		bool pause = false;
		bool view = false; // false: real layout, true: robots' knowledge of the layout

		// Main loop
		while (!quit) {
			ticks++;

			// Handle events
			while (SDL_PollEvent(&e) != 0) {
				// The close button
				if (e.type == SDL_QUIT) quit = true;

				else if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
					switch (e.key.keysym.sym) {
						// Pause by pressing ESC
					case SDLK_ESCAPE:
						pause = !pause;
						for (int i = 0; i < MAX_BUTTONS; i++) {
							if (buttons[i] != nullptr) buttons[i]->setShown();
						}
						break;
						// Move camera using arrow keys
					case SDLK_UP: camVelY -= camSpd; break;
					case SDLK_DOWN: camVelY += camSpd; break;
					case SDLK_LEFT: camVelX -= camSpd; break;
					case SDLK_RIGHT: camVelX += camSpd; break;
						// Zoom
					case SDLK_w: // Out
						if (SCREEN_SCALE > 0.5) SCREEN_SCALE -= 0.5;
						camera.w = (float)SCREEN_WIDTH / SCREEN_SCALE;
						camera.h = (float)SCREEN_HEIGHT / SCREEN_SCALE;
						break;
					case SDLK_e: // In
						if (SCREEN_SCALE < 5.5) SCREEN_SCALE += 0.5;
						camera.w = (float)SCREEN_WIDTH / SCREEN_SCALE;
						camera.h = (float)SCREEN_HEIGHT / SCREEN_SCALE;
						break;
					case SDLK_r: // Reset
						SCREEN_SCALE = 3;
						camera = { 0, 0, (float)SCREEN_WIDTH / SCREEN_SCALE, (float)SCREEN_HEIGHT / SCREEN_SCALE };
						break;
						// Switch between real layout and robots' knowledge of the layout
					case SDLK_TAB: view = !view; break;
					case SDLK_SPACE: skip = true; break;
					}
				}

				else if (e.type == SDL_KEYUP && e.key.repeat == 0) {
					switch (e.key.keysym.sym) {
					case SDLK_UP: camVelY += camSpd; break;
					case SDLK_DOWN: camVelY -= camSpd; break;
					case SDLK_LEFT: camVelX += camSpd; break;
					case SDLK_RIGHT: camVelX -= camSpd; break;
					}
				}

				// Resume button
				if (buttons[0]->isShown() && buttons[0]->handleEvents(e)) {
					pause = !pause;
					for (int i = 0; i < MAX_BUTTONS; i++) {
						if (buttons[i] != nullptr) buttons[i]->setShown();
					}
				}

				// Finish button
				if (buttons[1]->isShown() && buttons[1]->handleEvents(e)) {
					quit = true;
					finishSimulation = true;
				}

				// Quit button
				if (buttons[3]->isShown() && buttons[3]->handleEvents(e)) {
					quit = true;
					return 0;
				}

				// Menu button
				if (buttons[2]->isShown() && buttons[2]->handleEvents(e)) {
					quit = true;
					returnMenu = true;
				}
			}

			if (!pause) {
				// Obstacle tile generation and deletion at fixed tick intervals
				if (ticks % OBSTACLE_CD == 0) {
					// Reset all existing obstacle tiles to be floor tiles
					for (int i = 0; i < MAX_TILES; i++) {
						if (tiles[i] != nullptr) {
							if (tiles[i]->getType() == 9) tiles[i]->setTileType(1);
						}
					}

					// Randomly generate obstacle tiles
					for (int i = 0; i < NUMBER_OBSTACLES; i++) {
						// Look for a random floor tile (type == 1)
						bool valid = false;
						while (!valid) {
							int obstacleTile = rand() % MAX_TILES;

							if (tiles[obstacleTile] != nullptr) {
								if (tiles[obstacleTile]->getType() == 1) {
									valid = true;

									// Check that there are no robots on this tile
									for (int j = 0; j < NUMBER_ROBOTS; j++) {
										if (robots[j]->getBox().x == tiles[obstacleTile]->getX() && robots[j]->getBox().y == tiles[obstacleTile]->getY()) {
											valid = false;
											break;
										}
									}
								}

								if (valid) tiles[obstacleTile]->setTileType(9);
							}
						}
					}
				}

				// Process robots
				if (SDL_GetTicks64() - lastTick > TICK_INTERVAL) {
					// The entire decision and pathfinding algorithm is in this for-loop
					for (int i = 0; i < MAX_ROBOTS && !quit; i++) {
						if (robots[i] != nullptr) {
							if (robots[i]->getBattery() > 0) {
								float goalX = robots[i]->getBox().x;
								float goalY = robots[i]->getBox().y;
								double distance = std::numeric_limits<double>::infinity();
								int takeDir = -1;
								bool findShelf = false;
								bool explore = false;
								bool findExit = false;
								bool takeItemFromShelf = false;
								bool findCharger = false;
								bool waitingForCharger = false;
								bool chargeBattery = false;
								bool lookForNextCharger = false;
								bool submit = false;
								bool passItemAway = false;
								int recipientRobot = 0;
								int recipientSpace = 0;
								int passDir = 0;

								int rescueRobot = -1;
								for (int j = 0; j < MAX_ROBOTS; j++) {
									if (robots[j] != nullptr) {
										if (deadRobot[j] == i) {
											rescueRobot = j;
											break;
										}
									}
								}
								bool takeRobotItems = false;

								if (receivingItem[i]) {
									receivingItem[i] = false;
								}
								else {
									// Find smallest item in itemList
									int smallestItem = MAX_WEIGHT;
									for (int j = 0; j < NUMBER_ITEMS_RETRIEVE; j++) {
										if (weightOf(predItemList[j]) < weightOf(smallestItem) && predItemList[j] > 0) {
											smallestItem = predItemList[j];
											if (weightOf(smallestItem) == 1) break;
										}
									}

									// Charge until 100 if already charging
									if (tileDatabase[robots[i]->getTile(tileDatabase)] != nullptr) {
										if (tileDatabase[robots[i]->getTile(tileDatabase)]->getType() == 6 && robots[i]->getBattery() < 100) chargeBattery = true;
									}
									if (!chargeBattery) {
										// If robot is low on battery
										if (robots[i]->getBattery() < 50 || !chargerKnown) {
											double saveDistance = 0.0;
											// Look for the closest battery charger
											for (int j = 0; j < MAX_TILES; j++) {
												if (tileDatabase[j] != nullptr) {
													if (tileDatabase[j]->getType() == 6) {
														if (std::sqrt(pow(tileDatabase[j]->getX() - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() - robots[i]->getBox().y, 2)) < distance) {
															goalX = tileDatabase[j]->getX();
															goalY = tileDatabase[j]->getY();
															distance = std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2));
															findCharger = true;
															chargerKnown = true;
															lookForNextCharger = false;

															// If standing on charger, charge
															if (distance == 0) chargeBattery = true;
															// Check if some other robot is already on that charger
															else {
																for (int k = 0; k < NUMBER_ROBOTS; k++) {
																	if (robots[k] != nullptr) {
																		if (robots[k]->getBox().x == goalX && robots[k]->getBox().y == goalY) {
																			// Reset as if this charger is no longer a charger
																			findCharger = false;
																			lookForNextCharger = true;
																			saveDistance = distance;
																			distance = std::numeric_limits<double>::infinity();
																			break;
																		}
																	}
																}
															}
														}
													}
												}
											}
											
											if (!findCharger) {
												// Waiting for charger
												if (lookForNextCharger && saveDistance > WH) waitingForCharger = true;

												// If no known chargers, explore to look for one
												else explore = true;
											}
											else if (currentGoal[i] != 2) {
												currentGoal[i] = 2;
												robots[i]->resetHistory();
											}
										}
										// If robot is assigned to rescue a dead robot and has no items in hand
										else if (rescueRobot >= 0 && robots[i]->getWeight() == 0) {
											goalX = robots[rescueRobot]->getBox().x;
											goalY = robots[rescueRobot]->getBox().y;
											
											if (robots[i]->getBox().x == robots[rescueRobot]->getBox().x) {
												// If above dead robot
												if (robots[i]->getBox().y == robots[rescueRobot]->getBox().y - WH) {
													takeRobotItems = true;
													takeDir = 1;
												}
												// If below dead robot
												else if (robots[i]->getBox().y == robots[rescueRobot]->getBox().y + WH) {
													takeRobotItems = true;
													takeDir = 0;
												}
											}
											else if (robots[i]->getBox().y == robots[rescueRobot]->getBox().y) {
												// If to the left of dead robot
												if (robots[i]->getBox().x == robots[rescueRobot]->getBox().x - WH) {
													takeRobotItems = true;
													takeDir = 3;
												}
												// If to the right of dead robot
												else if (robots[i]->getBox().x == robots[rescueRobot]->getBox().x + WH) {
													takeRobotItems = true;
													takeDir = 2;
												}
											}

											if (currentGoal[i] != 4) {
												currentGoal[i] = 4;
												robots[i]->resetHistory();
											}
										}
										// If robot still has space for an item
										else if (robots[i]->getWeight() + weightOf(smallestItem) <= MAX_WEIGHT) {
											// Look for the closest shelf with an item in predItemList
											for (int j = 0; j < MAX_TILES; j++) {
												if (tileDatabase[j] != nullptr) {
													if (tileDatabase[j]->getType() >= 2 && tileDatabase[j]->getType() <= 5) {
														for (int k = 0; k < NUMBER_ITEMS_RETRIEVE; k++) {
															// If this item is in predItemList and the robot has space for it
															if (tileDatabase[j]->getItem() == predItemList[k] && robots[i]->getWeight() + tileDatabase[j]->getWeight() <= MAX_WEIGHT) {
																switch (tileDatabase[j]->getType()) {
																case 2:
																	if (std::sqrt(pow(tileDatabase[j]->getX() - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() - WH - robots[i]->getBox().y, 2)) < distance) {
																		goalX = tileDatabase[j]->getX();
																		goalY = tileDatabase[j]->getY() - WH;
																		takeDir = 1;
																	}
																	break;
																case 3:
																	if (std::sqrt(pow(tileDatabase[j]->getX() - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() + WH - robots[i]->getBox().y, 2)) < distance) {
																		goalX = tileDatabase[j]->getX();
																		goalY = tileDatabase[j]->getY() + WH;
																		takeDir = 0;
																	}
																	break;
																case 4:
																	if (std::sqrt(pow(tileDatabase[j]->getX() - WH - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() - robots[i]->getBox().y, 2)) < distance) {
																		goalX = tileDatabase[j]->getX() - WH;
																		goalY = tileDatabase[j]->getY();
																		takeDir = 3;
																	}
																	break;
																case 5:
																	if (std::sqrt(pow(tileDatabase[j]->getX() + WH - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() - robots[i]->getBox().y, 2)) < distance) {
																		goalX = tileDatabase[j]->getX() + WH;
																		goalY = tileDatabase[j]->getY();
																		takeDir = 2;
																	}
																	break;
																}
																distance = std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2));
																findShelf = true;
															}
														}
														if (distance == 0 && robots[i]->getWeight() + tileDatabase[j]->getWeight() <= MAX_WEIGHT && findShelf) {
															takeItemFromShelf = true;
															break;
														}
													}
												}
											}
											// If no known shelf with an item in predItemList exists, explore
											if (!findShelf) explore = true;
											else if (currentGoal[i] != 1) {
												currentGoal[i] = 1;
												robots[i]->resetHistory();
											}
										}
										else {
											// Look for closest exit
											for (int j = 0; j < MAX_TILES; j++) {
												if (tileDatabase[j] != nullptr) {
													if (tileDatabase[j]->getType() == 8) {
														if (std::sqrt(pow(tileDatabase[j]->getX() - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() - robots[i]->getBox().y, 2)) < distance) {
															goalX = tileDatabase[j]->getX();
															goalY = tileDatabase[j]->getY();
															distance = std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2));
															findExit = true;

															if (distance == 0 && robots[i]->getWeight() > 0) submit = true;
														}
													}
												}
											}
											// If no known exit in database, explore
											if (!findExit) explore = true;
											else if (currentGoal[i] != 3) {
												currentGoal[i] = 3;
												robots[i]->resetHistory();
											}
										}
									}
								}
								// Exploration
								if (explore) {
									// Look for the nearest unknown tile
									for (int j = 0; j < MAX_TILES; j++) {
										if (tileDatabase[j] != nullptr) {
											if (tileDatabase[j]->getType() == -1) {
												if (std::sqrt(pow(tiles[j]->getX() - robots[i]->getBox().x, 2) + pow(tiles[j]->getY() - robots[i]->getBox().y, 2)) < distance) {
													goalX = tiles[j]->getX();
													goalY = tiles[j]->getY();
													distance = std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2));
												}
											}
										}
									}

									if (currentGoal[i] != 0) {
										currentGoal[i] = 0;
										robots[i]->resetHistory();
									}
								}

								// Calculate f(n) = g(n) + h(n)
								double f[4] = { 0 };

								// where g(n) = visit history
								int currentTile = robots[i]->getTile(tileDatabase);
								int historyWeight = 100;

								f[0] += (double)(historyWeight * robots[i]->getHistory(currentTile - MAP_WIDTH / WH));
								f[1] += (double)(historyWeight * robots[i]->getHistory(currentTile + MAP_WIDTH / WH));
								f[2] += (double)(historyWeight * robots[i]->getHistory(currentTile - 1));
								f[3] += (double)(historyWeight * robots[i]->getHistory(currentTile + 1));

								// h(n) = Euclidean distance from goal
								f[0] -= std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y - WH, 2)); // Up
								f[1] -= std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y + WH, 2)); // Down
								f[2] -= std::sqrt(pow(goalX - robots[i]->getBox().x - WH, 2) + pow(goalY - robots[i]->getBox().y, 2)); // Left
								f[3] -= std::sqrt(pow(goalX - robots[i]->getBox().x + WH, 2) + pow(goalY - robots[i]->getBox().y, 2)); // Right

								// Check that robot is not moving to a blocked tile
								for (int j = 0; j < MAX_TILES; j++) {
									if (tileDatabase[j] != nullptr) {
										// Check if adjacent tile type is walkable tile type
										if (tileDatabase[j]->getType() != -1 && tileDatabase[j]->getType() != 1 && (tileDatabase[j]->getType() < 6 || tileDatabase[j]->getType() > 8)) {
											if (tileDatabase[j]->getX() == robots[i]->getBox().x) {
												if (tileDatabase[j]->getY() == robots[i]->getBox().y - WH) f[0] = std::numeric_limits<double>::infinity();
												else if (tileDatabase[j]->getY() == robots[i]->getBox().y + WH) f[1] = std::numeric_limits<double>::infinity();
											}
											else if (tileDatabase[j]->getY() == robots[i]->getBox().y) {
												if (tileDatabase[j]->getX() == robots[i]->getBox().x - WH) f[2] = std::numeric_limits<double>::infinity();
												else if (tileDatabase[j]->getX() == robots[i]->getBox().x + WH) f[3] = std::numeric_limits<double>::infinity();
											}
										}

										// Check if robot is at the edge of the map
										if (robots[i]->getBox().x == 0) f[2] = std::numeric_limits<double>::infinity();
										if (robots[i]->getBox().x == MAP_WIDTH - WH) f[3] = std::numeric_limits<double>::infinity();
										if (robots[i]->getBox().y == 0) f[0] = std::numeric_limits<double>::infinity();
										if (robots[i]->getBox().y == MAP_HEIGHT - WH) f[1] = std::numeric_limits<double>::infinity();
									}
								}

								// Choose minimum f(n)
								int bestAction = 0;
								for (int j = 1; j < 4; j++) {
									if (f[j] < f[bestAction]) {
										bestAction = j;
									}
								}

								// Check that that movement is not blocked by a robot
								// If blocked and finding exit, pass item to that robot
								int itemToPass = 0;
								for (int j = 0; j < NUMBER_ROBOTS; j++) {
									if (robots[j] != nullptr) {
										if (robots[j]->getBox().x == robots[i]->getBox().x) {
											if (robots[j]->getBox().y == robots[i]->getBox().y - WH) {
												f[0] = std::numeric_limits<double>::infinity();

												if (robots[j]->getBattery() > 0 && findExit && bestAction == 0 && robots[j]->getWeight() < MAX_WEIGHT && std::sqrt(pow(goalX - robots[j]->getBox().x, 2) + pow(goalY - robots[j]->getBox().y, 2)) < std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2))) {
													recipientSpace = MAX_WEIGHT - robots[recipientRobot]->getWeight();

													// Find largest item that can be passed
													for (int k = 0; k < MAX_WEIGHT; k++) {
														if (weightOf(robots[i]->getItem(k)) > weightOf(itemToPass) && weightOf(robots[i]->getItem(k)) <= recipientSpace) itemToPass = robots[i]->getItem(k);
													}

													if (itemToPass > 0) {
														passItemAway = true;
														recipientRobot = j;
														passDir = 0;
														break;
													}
												}
											}
											else if (robots[j]->getBox().y == robots[i]->getBox().y + WH) {
												f[1] = std::numeric_limits<double>::infinity();

												if (robots[j]->getBattery() > 0 && findExit && bestAction == 1 && robots[j]->getWeight() < MAX_WEIGHT && std::sqrt(pow(goalX - robots[j]->getBox().x, 2) + pow(goalY - robots[j]->getBox().y, 2)) < std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2))) {
													recipientSpace = MAX_WEIGHT - robots[recipientRobot]->getWeight();

													// Find largest item that can be passed
													for (int k = 0; k < MAX_WEIGHT; k++) {
														if (weightOf(robots[i]->getItem(k)) > weightOf(itemToPass) && weightOf(robots[i]->getItem(k)) <= recipientSpace) itemToPass = robots[i]->getItem(k);
													}

													if (itemToPass > 0) {
														passItemAway = true;
														recipientRobot = j;
														passDir = 1;
														break;
													}
												}
											}
										}
										else if (robots[j]->getBox().y == robots[i]->getBox().y) {
											if (robots[j]->getBox().x == robots[i]->getBox().x - WH) {
												f[2] = std::numeric_limits<double>::infinity();

												if (robots[j]->getBattery() > 0 && findExit && bestAction == 2 && robots[j]->getWeight() < MAX_WEIGHT && std::sqrt(pow(goalX - robots[j]->getBox().x, 2) + pow(goalY - robots[j]->getBox().y, 2)) < std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2))) {
													recipientSpace = MAX_WEIGHT - robots[recipientRobot]->getWeight();

													// Find largest item that can be passed
													for (int k = 0; k < MAX_WEIGHT; k++) {
														if (weightOf(robots[i]->getItem(k)) > weightOf(itemToPass) && weightOf(robots[i]->getItem(k)) <= recipientSpace) itemToPass = robots[i]->getItem(k);
													}

													if (itemToPass > 0) {
														passItemAway = true;
														recipientRobot = j;
														passDir = 2;
														break;
													}
												}
											}
											else if (robots[j]->getBox().x == robots[i]->getBox().x + WH) {
												f[3] = std::numeric_limits<double>::infinity();

												if (robots[j]->getBattery() > 0 && findExit && bestAction == 3 && robots[j]->getWeight() < MAX_WEIGHT && std::sqrt(pow(goalX - robots[j]->getBox().x, 2) + pow(goalY - robots[j]->getBox().y, 2)) < std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2))) {
													recipientSpace = MAX_WEIGHT - robots[recipientRobot]->getWeight();

													// Find largest item that can be passed
													for (int k = 0; k < MAX_WEIGHT; k++) {
														if (weightOf(robots[i]->getItem(k)) > weightOf(itemToPass) && weightOf(robots[i]->getItem(k)) <= recipientSpace) itemToPass = robots[i]->getItem(k);
													}

													if (itemToPass > 0) {
														passItemAway = true;
														recipientRobot = j;
														passDir = 3;
														break;
													}
												}
											}
										}
									}
								}

								// Choose minimum f(n)
								bestAction = 0;
								for (int j = 1; j < 4; j++) {
									if (f[j] < f[bestAction]) {
										bestAction = j;
									}
								}

								// Prints to check on robots
								/*printf("\nRobot %d\n", i);
								printf("\nStatus:\t [0]: %lf [1]: %lf [2]: %lf [3]: %lf\n", f[0], f[1], f[2], f[3]);
								printf("Weight: %d\n", robots[i]->getWeight());
								printf("Items: ");
								for (int j = 0; j < MAX_WEIGHT; j++) {
									printf("%d ", robots[i]->getItem(j));
								}
								printf("\n\tsubmit: %d\n", submit);
								printf("\tchargeBattery: %d\n", chargeBattery);
								printf("\ttakeItemFromShelf: %d\n", takeItemFromShelf);
								printf("\tfindShelf: %d\n", findShelf);
								printf("\tfindExit: %d\n", findExit);
								printf("\tfindCharger: %d\n", findCharger);
								printf("\texplore: %d\n", explore);
								printf("\treceivingItem: %d\n", receivingItem[i]);
								printf("\twaitingForCharger: %d\n", waitingForCharger);
								printf("\trescueRobot: %d\n", rescueRobot);*/
								//for (int j = 0; j < NUMBER_ITEMS_RETRIEVE; j++) {
								//	if (predItemList[j] != 0) printf("%d ", predItemList[j]);
								//}

								// Decide action
								// If waiting for charger, stay still
								// If receiving item, stay still
								if (!waitingForCharger && !receivingItem[i]) {
									if (submit) robots[i]->submitItems(tileDatabase, itemList);
									else if (chargeBattery) robots[i]->charge(tileDatabase);
									else if (takeRobotItems) {
										// Turn to dead robot if not already facing it
										if (robots[i]->getDir() != takeDir) robots[i]->turn(takeDir);
										// Take item from dead robot
										else robots[i]->takeRobotItem(tiles, robots);
									}
									else if (takeItemFromShelf) {
										// Turn to shelf if not already facing it
										if (robots[i]->getDir() != takeDir) robots[i]->turn(takeDir);
										// Take item from shelf
										else robots[i]->takeShelfItem(tiles, predItemList);
									}
									else if (passItemAway) {
										if (robots[i]->getDir() != passDir) robots[i]->turn(passDir);
										else {
											robots[i]->passItem(robots, tileDatabase, itemToPass);
											receivingItem[recipientRobot] = true;
										}
									}
									// Turn to direction if not already facing it
									else if (robots[i]->getDir() != bestAction) robots[i]->turn(bestAction);
									// Move
									else robots[i]->move(tiles, robots);
								}

								robots[i]->sight(tiles, tileDatabase);
								robots[i]->updateHistory(tileDatabase);

								// Count items retrieved
								itemsRetrieved = 0;
								for (int j = 0; j < NUMBER_ITEMS_RETRIEVE; j++) {
									if (itemList[j] == 0) itemsRetrieved++;
								}
								
								// Check if all items have been successfully retrieved
								if (itemsRetrieved == NUMBER_ITEMS_RETRIEVE) {
									// End the simulation
									printf("Completed!\n");
									if (saveResults) successfulRuns++;
									quit = true;
									finishSimulation = true;
								}

								// Count dead robots
								numDeadRobots = 0;
								for (int i = 0; i < MAX_ROBOTS; i++) {
									if (robots[i] != nullptr) {
										if (robots[i]->getBattery() <= 0) numDeadRobots++;
									}
								}

								// Check if run is doomed to fail or told to skip
								if (numDeadRobots > 0.5 * NUMBER_ROBOTS || skip || (ticks > 5000 && saveResults)) {
									// End the simulation
									printf("Failed!\n");
									if (saveResults) failedRuns++;
									quit = true;
									finishSimulation = true;
								}
							}
							// If robot battery is <= 0
							else {
								double distance = std::numeric_limits<double>::infinity();

								// Find closest surviving robot with sufficient battery and no items on hand
								for (int j = 0; j < MAX_ROBOTS; j++) {
									if (robots[j] != nullptr) {
										if (robots[j]->getBattery() >= 50 && robots[j]->getWeight() == 0) {
											if (std::sqrt(pow(robots[j]->getBox().x - robots[i]->getBox().x, 2) + pow(robots[j]->getBox().y - robots[i]->getBox().y, 2)) < distance) {
												distance = std::sqrt(pow(robots[j]->getBox().x - robots[i]->getBox().x, 2) + pow(robots[j]->getBox().y - robots[i]->getBox().y, 2));
												deadRobot[i] = j;
											}
										}
									}
								}
							}
						}
					}
					lastTick = SDL_GetTicks64();
				}

				// Process camera movement
				camera.x += camVelX;
				camera.y += camVelY;
			}

			// Reset screen
			SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
			SDL_RenderClear(renderer);

			// Render tiles
			if (view) {
				for (int i = 0; i < MAX_TILES; i++) {
					if (tiles[i] != nullptr) {
						tiles[i]->render(camera);
					}
				}
			}
			// Render robots' known tiles
			else {
				for (int i = 0; i < MAX_TILES; i++) {
					if (tileDatabase[i] != nullptr) {
						tileDatabase[i]->render(camera);
					}
				}
			}

			// Render robots
			for (int i = 0; i < NUMBER_ROBOTS; i++) {
				if (robots[i] != nullptr) {
					robots[i]->render(camera);
				}
			}

			// Darken screen when paused
			if (pause) {
				blackScreenTextureClip = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
				blackScreenTexture.render(0, 0, &blackScreenTextureClip);
			}

			// Render buttons
			for (int i = 0; i < MAX_BUTTONS; i++) {
				if (buttons[i] != nullptr) {
					buttons[i]->render();
				}
			}

			// Render "Paused"
			if (pause) renderTitle("Paused", (float)SCREEN_WIDTH / 2, 150, true, true);

			// Update the screen
			SDL_RenderPresent(renderer);
		}
	}

	// Delete tiles
	for (int i = 0; i < MAX_TILES; i++) {
		if (tiles[i] != nullptr) {
			delete tiles[i];
			tiles[i] = nullptr;
		}

		if (tileDatabase[i] != nullptr) {
			delete tileDatabase[i];
			tileDatabase[i] = nullptr;
		}
	}

	// Delete robots
	for (int i = 0; i < NUMBER_ROBOTS; i++) {
		if (robots[i] != nullptr) {
			delete robots[i];
			robots[i] = nullptr;
		}
	}

	// Delete buttons
	for (int i = 0; i < MAX_BUTTONS; i++) {
		if (buttons[i] != nullptr) {
			delete buttons[i];
			buttons[i] = nullptr;
		}
	}

	if (returnMenu) menu();
	else if (finishSimulation) {
		itemsRetrieved = 0;
		for (int i = 0; i < NUMBER_ITEMS_RETRIEVE; i++) {
			if (itemList[i] == 0) itemsRetrieved++;
		}

		if (!saveResults) {
			// Display results
			printf("Total Ticks: %d\n", ticks);
			if (itemsRetrieved > 0) printf("Average ticks per item: %f\n", (float)ticks / (float)itemsRetrieved);
			printf("Items retrieved: %d\n", itemsRetrieved);
			printf("Number of dead robots: %d\n", numDeadRobots);
			printf("Simulation run time: %f\n", (float)(SDL_GetTicks64() - runtime) / (float)1000);
		}
		else if (itemsRetrieved == 100) {
			// Save results if run was successful
			ticksTaken[iteration] = ticks;
			ticksTakenPerItem[iteration] = (float)ticks / (float)itemsRetrieved;
			numberDeadRobots[iteration] = numDeadRobots;
			timeTaken[iteration] = (float)(SDL_GetTicks64() - runtime) / (float)1000;
		}
		printf("-------------------------------------------\n");

		if (skip) return 2;
	}

	return 1;
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

	// Open the warehouse robot simulation menu
	menu();

	// Close SDL library
	closeSDL();

	// Prevent program from closing immediately
	printf("\nEnter any number to quit\n> ");
	int i = 0;
	std::cin >> i;

	return 0;
}