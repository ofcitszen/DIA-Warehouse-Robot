#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Screen sizes
int SCREEN_WIDTH = 1280;
int SCREEN_HEIGHT = 720;
float SCREEN_SCALE = 3;

// Number of items to be retrieved
constexpr int ITEMS_TO_RETRIEVE = 100;

// Number of floors
constexpr int NUMBER_OF_FLOORS = 4;

// Tile width and height
constexpr int WH = 16;
// Number of tile sprites
constexpr int TILE_SPRITES = 9;
// Maximum number of tiles
constexpr int MAX_TILES = 10000;
// Map width
int MAP_WIDTH = 50 * WH;
int MAP_HEIGHT = 50 * WH * 4;

// Number of robot sprites
constexpr int ROBOT_SPRITES = 4;
// Maximum number of robots
constexpr int MAX_ROBOTS = 5;
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
Uint64 TICK_INTERVAL = 0;

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
	Tile(float x, float y, int setType) {
		hitbox = { x, y, WH, WH };
		type = setType;
		
		if (type >= 2 && type <= 5) {
			item = rand() % 10 + 1;
		}
		else item = -1;
	}
	void render(SDL_FRect& camera) {
		if (type != 0) {
			if (SDL_HasIntersectionF(&hitbox, &camera)) {
				tilesTexture.render(hitbox.x - camera.x, hitbox.y - camera.y, &tilesTextureClips[type - 1]);

				// If shelf tile, show item contained in it
				//if (type >= 2 && type <= 5) {
				//	// Load text to get its dimensions
				//	textTexture.loadText(std::to_string(item), pixellari);

				//	// Render text in the center of the shelf
				//	renderText(std::to_string(item), (float)hitbox.x + (float)hitbox.w / 2 - camera.x * SCREEN_SCALE, (float)hitbox.y + (float)hitbox.h / 2 - camera.y * SCREEN_SCALE, true, true);
				//}
			}
		}
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
	int getItem() {
		return item;
	}
private:
	SDL_FRect hitbox;
	int type;
	int item;	// Item held by shelves; 0 if no item or not a shelf
};

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
		for (int i = 0; i < MAX_WEIGHT; i++) {
			if (items[i] == 0) {
				items[i] = item;
				weight += items[i];
				break;
			}
		}
	}

	// Set functions
	void setXY(float x, float y) {
		hitbox.x = x;
		hitbox.y = y;
	}
	void setItems(int setItems[]) {
		weight = 0;
		for (int i = 0; i < MAX_WEIGHT; i++) {
			items[i] = setItems[i];
			weight += setItems[i];
		}
	}
	void setDir(int direction) {
		dir = direction;
	}
	void setBattery(float batteryLevel) {
		battery = batteryLevel;
	}

	// Actions
	bool turn(int direction) {
		if (battery > 0 && dir != direction) {
			dir = direction;

			// Decrement battery
			battery -= BATTERY_LOSS;

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
		bool elevator = false;

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

								// Set flag for elevators
								if (tiles[i]->getType() == 7) {
									elevator = true;
								}
								break;
							}
						}
					}
				}

				int instances = 0;
				if (success) {
					for (int i = 0; i < MAX_ROBOTS; i++) {
						if (robots[i] != nullptr) {
							SDL_FRect robotHitbox = robots[i]->getBox();
							SDL_FRect selfHitbox = hitbox;
							
							// Check all floors for elevator
							if (elevator) {
								robotHitbox.y = (float)((int)robotHitbox.y % (MAP_HEIGHT / NUMBER_OF_FLOORS));
								selfHitbox.y = (float)((int)selfHitbox.y % (MAP_HEIGHT / NUMBER_OF_FLOORS));
							}

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
	bool takeShelfItem(Tile* tiles[]) {
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
						if (weight + tiles[currentTile - map_width]->getItem() > MAX_WEIGHT) return false;

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
						if (weight + tiles[currentTile + map_width]->getItem() > MAX_WEIGHT) return false;

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
						if (weight + tiles[currentTile - 1]->getItem() > MAX_WEIGHT) return false;

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
						if (weight + tiles[currentTile + 1]->getItem() > MAX_WEIGHT) return false;

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

		// Set sprite based on battery
		if (battery == 0) sprite = 0;
		else if (battery < 20) sprite = 1;
		else if (battery < 50) sprite = 2;

		return true;
	}
	bool charge(Tile* tiles[]) {
		// If standing on a charger tile, increase battery level
		if (tiles[getTile(tiles)] != nullptr) {
			if (tiles[getTile(tiles)]->getType() == 6) {
				battery += BATTERY_GAIN;
				if (battery > 100) battery = 100;
				return true;
			}
		}
		return false;
	}
	bool useElevator(Tile* tiles[], int updown) {
		int currentTile = getTile(tiles);
		int map_width = MAP_WIDTH / WH;
		int map_height = MAP_HEIGHT / WH;

		// If standing on an elevator tile
		if (tiles[getTile(tiles)] != nullptr) {
			if (tiles[getTile(tiles)]->getType() == 7) {
				switch (updown) {
				case 0: // Up
					// Check bounds
					if ((currentTile + map_width * map_height / NUMBER_OF_FLOORS < map_width * map_height) && tiles[currentTile + map_width * map_height / NUMBER_OF_FLOORS] != nullptr) {
						hitbox.y += MAP_HEIGHT / NUMBER_OF_FLOORS;

						battery -= BATTERY_LOSS;

						// Set sprite based on battery
						if (battery == 0) sprite = 0;
						else if (battery < 20) sprite = 1;
						else if (battery < 50) sprite = 2;

						return true;
					}
					break;
				case 1: // Down
					// Check bounds
					if ((currentTile - map_width * map_height / NUMBER_OF_FLOORS > 0) && tiles[currentTile - map_width * map_height / NUMBER_OF_FLOORS] != nullptr) {
						hitbox.y -= MAP_HEIGHT / NUMBER_OF_FLOORS;

						battery -= BATTERY_LOSS;

						// Set sprite based on battery
						if (battery == 0) sprite = 0;
						else if (battery < 20) sprite = 1;
						else if (battery < 50) sprite = 2;

						return true;
					}
				}
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
				for (int i = 0; i < MAX_ROBOTS; i++) {
					if (robots[i] != nullptr) {
						// If this is the right robot
						if (robots[i]->getBox().x == hitbox.x && robots[i]->getBox().y == hitbox.y - WH) {
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
				for (int i = 0; i < MAX_ROBOTS; i++) {
					if (robots[i] != nullptr) {
						// If this is the right robot
						if (robots[i]->getBox().x == hitbox.x && robots[i]->getBox().y == hitbox.y + WH) {
							// If that robot has room for the item
							if (robots[i]->getWeight() <= MAX_WEIGHT - item) {
								// Remove item from this robot
								for (int j = 0; j < MAX_WEIGHT; j++) {
									if (items[j] == item) {
										items[j] = 0;
									}
								}

								// Pass the item to that robot
								robots[i]->addItem(item);

								battery -= BATTERY_LOSS;

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
				for (int i = 0; i < MAX_ROBOTS; i++) {
					if (robots[i] != nullptr) {
						// If this is the right robot
						if (robots[i]->getBox().x == hitbox.x - WH && robots[i]->getBox().y == hitbox.y) {
							// If that robot has room for the item
							if (robots[i]->getWeight() <= MAX_WEIGHT - item) {
								// Remove item from this robot
								for (int j = 0; j < MAX_WEIGHT; j++) {
									if (items[j] == item) {
										items[j] = 0;
									}
								}

								// Pass the item to that robot
								robots[i]->addItem(item);

								battery -= BATTERY_LOSS;

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
				for (int i = 0; i < MAX_ROBOTS; i++) {
					if (robots[i] != nullptr) {
						// If this is the right robot
						if (robots[i]->getBox().x == hitbox.x + WH && robots[i]->getBox().y == hitbox.y) {
							// If that robot has room for the item
							if (robots[i]->getWeight() <= MAX_WEIGHT - item) {
								// Remove item from this robot
								for (int j = 0; j < MAX_WEIGHT; j++) {
									if (items[j] == item) {
										items[j] = 0;
									}
								}

								// Pass the item to that robot
								robots[i]->addItem(item);

								battery -= BATTERY_LOSS;

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
	bool submitItems(Tile* tiles[], int* itemList) {
		bool success = false;
		// If standing on a submission tile
		if (tiles[getTile(tiles)] != nullptr) {
			if (tiles[getTile(tiles)]->getType() == 8) {
				for (int j = 0; j < MAX_WEIGHT; j++) {
					if (items[j] != 0) {
						for (int i = 0; i < ITEMS_TO_RETRIEVE; i++) {
							if (itemList[i] != 0) {
								if (items[j] == itemList[i]) {
									// Submit the item
									items[j] = 0;
									itemList[i] = 0;
									weight -= items[j];

									battery -= BATTERY_LOSS;

									// Set sprite based on battery
									if (battery == 0) sprite = 0;
									else if (battery < 20) sprite = 1;
									else if (battery < 50) sprite = 2;

									success = true;
								}
							}
						}
					}
				}
			}
		}
		return success;
	}

	double sight(Tile* tiles[], Tile* tileDatabase[]) {
		double reward = 0;

		int currentTile = getTile(tiles);
		int sightRange = 10;
		int map_width = MAP_WIDTH / WH;
		int map_height = MAP_HEIGHT / WH;

		bool stop = false;
		for (int i = 0; i < sightRange && !stop; i++) {
			// Record tile in database
			if (tileDatabase[currentTile] == nullptr) {
				tileDatabase[currentTile] = tiles[currentTile];
				reward += 10;
			}

			// Stop if this tile is a shelf
			if (tiles[currentTile] != nullptr) {
				if (tiles[currentTile]->getType() >= 2 && tiles[currentTile]->getType() <= 5 || tiles[currentTile]->getType() == 0) break;
			}

			// Stop if next tile is out of bounds
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
		return reward;
	}
	void updateHistory(Tile* tiles[]) {
		int currentTile = getTile(tiles);

		if (tiles[currentTile] != nullptr) visitHistory[currentTile]++;
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

// Node struct
struct Node {
	std::vector<Node*> children;
	Node* parent = nullptr;
	double visits = 0;
	double UCBval = 0.0;

	// Game state
	int action = 0;
	Tile* tileDatabase[MAX_TILES] = { nullptr };
	Robot* robots[MAX_ROBOTS] = { nullptr };
	int itemList[ITEMS_TO_RETRIEVE] = { 0 };

	Node(Tile* setTileDatabase[], Robot* setRobots[], int setItemList[], int setAction = 0, Node* setParent = nullptr) {
		for (int i = 0; i < MAX_TILES; i++) {
			if (setTileDatabase[i] != nullptr) {
				tileDatabase[i] = setTileDatabase[i];
			}
		}

		for (int i = 0; i < MAX_ROBOTS; i++) {
			if (setRobots[i] != nullptr) {
				robots[i] = setRobots[i];
			}
		}

		for (int i = 0; i < ITEMS_TO_RETRIEVE; i++) itemList[i] = setItemList[i];

		action = setAction;
		parent = setParent;
	}

	// Destructor to recursively delete child nodes
	~Node() {
		for (Node* child : children) {
			delete child; // Delete each child node
			child = nullptr;
		}
	}
};

// MCTS
class MCTS {
public:
	MCTS(int numOfSimulations, int robotNumber) {
		numSim = numOfSimulations;
		robotNum = robotNumber;
	}

	Node* run(Node* root) {
		for (int i = 0; i < numSim; i++) {
			Node* selected = select(root);
			Node* expanded = expand(selected);
			double reward = simulate(expanded);
			backpropagate(expanded, reward);
		}
		return bestChild(root);
	}
private:
	int numSim;
	int robotNum;

	// Select the child with the highest Upper Confidence Bound (UCB) value and return it
	Node* select(Node* node) {
		while (!node->children.empty()) {
			node = bestUCT(node);
		}
		return node;
	}

	// Expand the current node by generating all possible moves as child nodes
	Node* expand(Node* node) {
		// Save game state
		float x = node->robots[robotNum]->getBox().x;
		float y = node->robots[robotNum]->getBox().y;
		int items[MAX_WEIGHT] = { 0 };
		for (int i = 0; i < MAX_WEIGHT; i++) {
			items[i] = node->robots[robotNum]->getItem(i);
		}
		int dir = node->robots[robotNum]->getDir();
		int itemList[MAX_WEIGHT] = { 0 };
		for (int i = 0; i < MAX_WEIGHT; i++) {
			itemList[i] = node->itemList[i];
		}
		float battery = node->robots[robotNum]->getBattery();

		for (int i = 0; i < 11; i++) {
			bool success = false;

			if (i >= 0 && i <= 3) {
				success = node->robots[robotNum]->turn(i);
			}
			if (i == 4) {
				success = node->robots[robotNum]->move(node->tileDatabase, node->robots);
			}
			if (i == 5) {
				success = node->robots[robotNum]->takeShelfItem(node->tileDatabase);
			}
			if (i == 6) {
				success = node->robots[robotNum]->charge(node->tileDatabase);
			}
			if (i >= 7 && i <= 8) {
				success = node->robots[robotNum]->useElevator(node->tileDatabase, i - 7);
			}
			/*if (i >= 12 && i <= 51) {
				success = robots[robotNum]->passItem(robots, tiles, (i - 12) % 10 + 1);
			}*/
			if (i == 9) {
				success = node->robots[robotNum]->submitItems(node->tileDatabase, node->itemList);
			}
			if (i == 10) {
				success = true; // Do nothing
			}

			if (success) {
				Node* child = new Node(node->tileDatabase, node->robots, node->itemList, i, node);
				node->children.push_back(child);
			}

			// Reset this node's game state
			node->robots[robotNum]->setXY(x, y);
			node->robots[robotNum]->setItems(items);
			node->robots[robotNum]->setDir(dir);
			for (int j = 0; j < MAX_WEIGHT; j++) {
				node->itemList[j] = itemList[j];
			}
			node->robots[robotNum]->setBattery(battery);
		}
		return node->children[std::rand() % node->children.size()]; // Return a random child node for simulate step
	}

	// Simulate a random rollout path from the current node
	double simulate(Node* node) {
		double reward = 0;
		int ticks = 0;
		float goalX = 0; float goalY = 0;
		float currentX = 0; float currentY = 0;
		double minDistanceFromGoal = std::numeric_limits<double>::infinity();
		double distance = 0;

		// Save game state
		float x = node->robots[robotNum]->getBox().x;
		float y = node->robots[robotNum]->getBox().y;
		int items[MAX_WEIGHT] = { 0 };
		for (int i = 0; i < MAX_WEIGHT; i++) {
			items[i] = node->robots[robotNum]->getItem(i);
		}
		int dir = node->robots[robotNum]->getDir();
		int itemList[MAX_WEIGHT] = { 0 };
		for (int i = 0; i < MAX_WEIGHT; i++) {
			itemList[i] = node->itemList[i];
		}
		float battery = node->robots[robotNum]->getBattery();

		// Simulation
		bool stop = false;
		while (!stop) {
			int action = rand() % 11;

			bool success = false;

			if (action >= 0 && action <= 3) {
				success = node->robots[robotNum]->turn(action);
			}
			if (action == 4) {
				success = node->robots[robotNum]->move(node->tileDatabase, node->robots);
				if (success) reward += 1;
			}
			if (action == 5) {
				success = node->robots[robotNum]->takeShelfItem(node->tileDatabase);
			}
			if (action == 6) {
				success = node->robots[robotNum]->charge(node->tileDatabase);
			}
			if (action >= 7 && action <= 8) {
				success = node->robots[robotNum]->useElevator(node->tileDatabase, action - 7);
			}
			/*if (action >= 12 && action <= 51) {
				success = robots[robotNum]->passItem(robots, tiles, (action - 12) % 10 + 1);
			}*/
			if (action == 9) {
				success = node->robots[robotNum]->submitItems(node->tileDatabase, node->itemList);
				//if (success) reward += 100;
			}
			if (action == 10) {
				success = true; // Do nothing
			}

			// Break if illegal move
			if (!success) break;

			// Increment ticks
			ticks++;

			minDistanceFromGoal = std::numeric_limits<double>::infinity();
			if (node->robots[robotNum]->getWeight() >= 0) {
				for (int i = 0; i < MAX_TILES; i++) {
					if (node->tileDatabase[i] != nullptr) {
						if (node->tileDatabase[i]->getType() == 8) {
							distance = std::sqrt(pow(currentX - node->tileDatabase[i]->getX(), 2) + pow(currentY - node->tileDatabase[i]->getY(), 2));

							// Save minimum distance
							if (distance < minDistanceFromGoal) {
								minDistanceFromGoal = distance;
							}
						}
					}
				}
			}
			//reward -= ticks;
			if (minDistanceFromGoal < std::numeric_limits<double>::infinity()) reward -= minDistanceFromGoal;

			// Check for termination criteria
			stop = true;
			for (int i = 0; i < ITEMS_TO_RETRIEVE; i++) {
				if (itemList[i] != 0) {
					stop = false;
					break;
				}
			}
			// Stop at a certain number of ticks into the simulation
			if (ticks >= 50) stop = true;
		}

		// Reset this node's game state
		node->robots[robotNum]->setXY(x, y);
		node->robots[robotNum]->setItems(items);
		node->robots[robotNum]->setDir(dir);
		for (int j = 0; j < MAX_WEIGHT; j++) {
			node->itemList[j] = itemList[j];
		}
		node->robots[robotNum]->setBattery(battery);

		return reward;
	}

	// Backpropagate the reward through the tree, updating visits and value
	void backpropagate(Node* node, double reward) {
		while (node != nullptr) {
			node->visits++; // Increment the visit count
			node->UCBval += reward; // Add the reward value
			node = node->parent; // Move up to the parent node
		}
	}

	// Find the child with the best Upper Confidence Bound (UCB) value
	Node* bestUCT(Node* node) {
		Node* best = nullptr;
		double bestValue = -std::numeric_limits<double>::infinity();

		for (Node* child : node->children) {
			double uctValue = child->UCBval / (child->visits) + 0.5 *
				std::sqrt(2.0 * std::log(node->visits) / (child->visits));
			if (uctValue > bestValue) {
				bestValue = uctValue;
				best = child;
			}
		}
		return best;
	}

	// Select the child node with the best average reward
	Node* bestChild(Node* node) {
		Node* best = nullptr;
		double bestValue = -std::numeric_limits<double>::infinity();

		for (Node* child : node->children) {
			double value = child->UCBval / (child->visits); // Calculate average reward
			if (value > bestValue) {
				bestValue = value;
				best = child;
			}
		}
		return best;
	}
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

// Just function declarations
void simulation();

// Main menu
void menu() {
	// Initialise variables
	SDL_Event e;

	// Initialise buttons
	Button* buttons[MAX_BUTTONS] = { nullptr };
	buttons[0] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, "Start");
	buttons[1] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 150, "Train");
	buttons[2] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 2 * 150, "Settings");
	buttons[3] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 3 * 150, "Quit");
	buttons[4] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 3 * 150, "Back");
	buttons[4]->setShown();

	// Main loop
	bool quit = false;
	bool startSimulation = false;
	bool startTraining = false;
	bool changeSettings = false;
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

			// Start button
			if (buttons[1]->isShown() && buttons[1]->handleEvents(e)) {
				quit = true;
				startTraining = true;
			}

			// Settings button and Back button
			if (buttons[2]->isShown() && buttons[2]->handleEvents(e) || buttons[4]->isShown() && buttons[4]->handleEvents(e)) {
				changeSettings = !changeSettings;
				for (int i = 0; i < 5; i++) buttons[i]->setShown();
			}

			// Quit button
			if (buttons[3]->isShown() && buttons[3]->handleEvents(e)) quit = true;
		}

		// Reset screen
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(renderer);
		
		// Render title
		if (!changeSettings) renderTitle("Warehouse Robot Simulation", (float)SCREEN_WIDTH / 2, 300, true);
		else renderTitle("Settings", (float)SCREEN_WIDTH / 2, 150, true);

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

	if (startSimulation) simulation();
}

// Main simulation code
void simulation() {
	// Initialise variables
	SDL_FRect camera = { 0, 0, (float)SCREEN_WIDTH / SCREEN_SCALE, (float)SCREEN_HEIGHT / SCREEN_SCALE };
	float camSpd = 10;
	float camVelX = 0;
	float camVelY = 0;
	SDL_Event e;

	// Results screen variables
	int ticks = 0;
	int itemsRetrieved = 0;

	// Initialise objects
	Tile* tiles[MAX_TILES] = { nullptr };
	Tile* tileDatabase[MAX_TILES] = { nullptr };
	Robot* robots[MAX_ROBOTS] = { nullptr };
	Button* buttons[MAX_BUTTONS] = { nullptr };
	buttons[0] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, "Resume");
	buttons[1] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 150, "Menu");
	buttons[2] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 2 * 150, "Finish");
	buttons[3] = new Button(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 3 * 150, "Quit");
	for (int i = 0; i < 4; i++) buttons[i]->setShown();

	// Random seed
	srand((unsigned int)time(0));

	// List of items to retrieve
	int itemList[ITEMS_TO_RETRIEVE] = {0};
	for (int i = 0; i < ITEMS_TO_RETRIEVE; i++) {
		itemList[i] = rand() % 10 + 1;
	}

	// Event flag
	bool returnMenu = false;
	bool finishSimulation = false;

	// Create tiles based on map
	if (!setTiles(tiles, "warehouse_resources/map1.map", 50 * WH, 50 * WH * 4)) printf("setTiles() error\n");
	else {
		// Create robots
		for (int i = 0; i < MAX_ROBOTS; i++) {
			robots[i] = new Robot((float)(WH * (rand() % 50)), (float)(WH * (rand() % 50)), rand() % 4);
		}

		// Time control
		Uint64 lastTick = 0;

		// Main loop
		bool quit = false;
		bool pause = false;
		bool view = false; // false: real layout, true: robots' knowledge of the layout

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
						for (int i = 0; i < 4; i++) buttons[i]->setShown();
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
					for (int i = 0; i < 4; i++) buttons[i]->setShown();
				}

				// Menu button
				if (buttons[1]->isShown() && buttons[1]->handleEvents(e)) {
					quit = true;
					returnMenu = true;
				}

				// Finish button
				if (buttons[2]->isShown() && buttons[2]->handleEvents(e)) {
					quit = true;
					finishSimulation = true;
				}

				// Quit button
				if (buttons[3]->isShown() && buttons[3]->handleEvents(e)) quit = true;
			}

			if (!pause) {
				// Process robots
				if (SDL_GetTicks64() - lastTick > TICK_INTERVAL) {
					bool movementBlocked[MAX_ROBOTS] = { false };
					for (int i = 0; i < MAX_ROBOTS; i++) {
						if (robots[i] != nullptr) {
							float goalX = 0;
							float goalY = 0;
							double distance = std::numeric_limits<double>::infinity();
							int takeDir = -1;
							bool findShelf = false;
							bool explore = false;
							bool findExit = false;
							bool takeItemFromShelf = false;
							bool findCharger = false;
							bool chargeBattery = false;
							bool submit = false;

							// Find smallest item in itemList
							int smallestItem = 11;
							for (int j = 0; j < ITEMS_TO_RETRIEVE; j++) {
								if (itemList[j] < smallestItem && itemList[j] > 0) {
									smallestItem = itemList[j];
									if (itemList[j] == 1) break;
								}
							}

							// Charge until 100 if already charging
							if (!(chargeBattery && robots[i]->getBattery() < 100)) {
								chargeBattery = false;

								// If robot is low on battery
								if (robots[i]->getBattery() < 25) {
									// Look for the closest battery charger
									for (int j = 0; j < MAX_TILES; j++) {
										if (tileDatabase[j] != nullptr) {
											if (tileDatabase[j]->getType() == 6) {
												if (std::sqrt(pow(tileDatabase[j]->getX() - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() - robots[i]->getBox().y, 2)) < distance) {
													goalX = tileDatabase[j]->getX();
													goalY = tileDatabase[j]->getY();
													distance = std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2));
													findCharger = true;

													if (distance == 0) chargeBattery = true;
												}
											}
										}
									}
									if (!findCharger) explore = true;
								}
								// If robot still has space for an item
								else if (robots[i]->getWeight() + smallestItem < MAX_WEIGHT) {
									// Look for the closest shelf with an item in itemList
									for (int j = 0; j < MAX_TILES; j++) {
										if (tileDatabase[j] != nullptr) {
											if (tileDatabase[j]->getType() >= 2 && tileDatabase[j]->getType() <= 5) {
												for (int k = 0; k < ITEMS_TO_RETRIEVE; k++) {
													if (tileDatabase[j]->getItem() == itemList[k]) {
														switch (tileDatabase[j]->getType()) {
														case 2:
															if (std::sqrt(pow(tileDatabase[j]->getX() - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() - WH - robots[i]->getBox().y, 2)) < distance) {
																goalX = tileDatabase[j]->getX();
																goalY = tileDatabase[j]->getY() - WH;
																distance = std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2));
																takeDir = 1;
															}
															break;
														case 3:
															if (std::sqrt(pow(tileDatabase[j]->getX() - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() + WH - robots[i]->getBox().y, 2)) < distance) {
																goalX = tileDatabase[j]->getX();
																goalY = tileDatabase[j]->getY() + WH;
																distance = std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2));
																takeDir = 0;
															}
															break;
														case 4:
															if (std::sqrt(pow(tileDatabase[j]->getX() - WH - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() - robots[i]->getBox().y, 2)) < distance) {
																goalX = tileDatabase[j]->getX() - WH;
																goalY = tileDatabase[j]->getY();
																distance = std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2));
																takeDir = 3;
															}
															break;
														case 5:
															if (std::sqrt(pow(tileDatabase[j]->getX() + WH - robots[i]->getBox().x, 2) + pow(tileDatabase[j]->getY() - robots[i]->getBox().y, 2)) < distance) {
																goalX = tileDatabase[j]->getX() + WH;
																goalY = tileDatabase[j]->getY();
																distance = std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2));
																takeDir = 2;
															}
															break;
														}
														findShelf = true;
													}
												}
												if (distance == 0 && robots[i]->getWeight() + tileDatabase[j]->getItem() < MAX_WEIGHT) {
													takeItemFromShelf = true;
													break;
												}
											}
										}
									}
									// If no known shelf with an item in itemList exists, explore
									if (!findShelf) explore = true;
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

													if (distance < WH) submit = true;
												}
											}
										}
									}
									// If no known exit in database, explore
									if (!findExit) explore = true;
								}
							}

							// Exploration
							if (explore) {
								// Look for the nearest unknown tile
								for (int j = 0; j < MAX_TILES; j++) {
									if (tileDatabase[j] == nullptr) {
										if (std::sqrt(pow(tiles[j]->getX() - robots[i]->getBox().x, 2) + pow(tiles[j]->getY() - robots[i]->getBox().y, 2)) < distance) {
											goalX = tiles[j]->getX();
											goalY = tiles[j]->getY();
											distance = std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y, 2));
										}
									}
								}
							}
							
							// Calculate f(n) = g(n) + h(n)
							double f[4] = { 0 };

							// where g(n) = visit history
							int currentTile = robots[i]->getTile(tileDatabase);

							f[0] += (double)(robots[i]->getHistory(currentTile - MAP_WIDTH / WH));
							f[1] += (double)(robots[i]->getHistory(currentTile + MAP_WIDTH / WH));
							f[2] += (double)(robots[i]->getHistory(currentTile - 1));
							f[3] += (double)(robots[i]->getHistory(currentTile + 1));

							// h(n) = Euclidean distance from goal
							f[0] -= std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y - WH, 2)); // Up
							f[1] -= std::sqrt(pow(goalX - robots[i]->getBox().x, 2) + pow(goalY - robots[i]->getBox().y + WH, 2)); // Down
							f[2] -= std::sqrt(pow(goalX - robots[i]->getBox().x - WH, 2) + pow(goalY - robots[i]->getBox().y, 2)); // Left
							f[3] -= std::sqrt(pow(goalX - robots[i]->getBox().x + WH, 2) + pow(goalY - robots[i]->getBox().y, 2)); // Right

							// Check that that movement is not a blocked tile
							for (int j = 0; j < MAX_TILES; j++) {
								if (tileDatabase[j] != nullptr) {
									if (tileDatabase[j]->getType() != 1 && (tileDatabase[j]->getType() < 6 || tileDatabase[j]->getType() > 8)) {
										if (tileDatabase[j]->getX() == robots[i]->getBox().x) {
											if (tileDatabase[j]->getY() == robots[i]->getBox().y - WH) f[0] = std::numeric_limits<double>::infinity();
											else if (tileDatabase[j]->getY() == robots[i]->getBox().y + WH) f[1] = std::numeric_limits<double>::infinity();
										}
										else if (tileDatabase[j]->getY() == robots[i]->getBox().y) {
											if (tileDatabase[j]->getX() == robots[i]->getBox().x - WH) f[2] = std::numeric_limits<double>::infinity();
											else if (tileDatabase[j]->getX() == robots[i]->getBox().x + WH) f[3] = std::numeric_limits<double>::infinity();
										}
									}
								}
							}
							for (int j = 0; j < MAX_ROBOTS; j++) {
								if (robots[j] != nullptr) {
									if (robots[j]->getBox().x == robots[i]->getBox().x) {
										if (robots[j]->getBox().y == robots[i]->getBox().y - WH) f[0] = std::numeric_limits<double>::infinity();
										else if (robots[j]->getBox().y == robots[i]->getBox().y + WH) f[1] = std::numeric_limits<double>::infinity();
									}
									else if (robots[j]->getBox().y == robots[i]->getBox().y) {
										if (robots[j]->getBox().x == robots[i]->getBox().x - WH) f[2] = std::numeric_limits<double>::infinity();
										else if (robots[j]->getBox().x == robots[i]->getBox().x + WH) f[3] = std::numeric_limits<double>::infinity();
									}
								}
							}
							
							// Choose minimum f(n)
							int bestAction = 0;
							for (int j = 1; j < 4; j++) {
								if (f[j] < f[bestAction]) {
									bestAction = j;
								}
							}

							// Decide action
							if (submit) robots[i]->submitItems(tileDatabase, itemList);
							else if (chargeBattery) robots[i]->charge(tileDatabase);
							else if (takeItemFromShelf) {
								// Turn to shelf if not already facing it
								if (robots[i]->getDir() != takeDir) robots[i]->turn(takeDir);
								// Take item from shelf
								else robots[i]->takeShelfItem(tiles);
							}
							// Turn to direction if not already facing it
							else if (robots[i]->getDir() != bestAction) robots[i]->turn(bestAction);
							// Move
							else robots[i]->move(tiles, robots);

							/*Node* root = new Node(tileDatabase, robots, itemList);
							MCTS mcts(50, i);
							Node* best = mcts.run(root);
							int action = best->action;*/

							//// Process action
							//if (action >= 0 && action <= 3) {
							//	robots[i]->turn(action);
							//}
							//if (action == 4) {
							//	robots[i]->move(tiles, robots);
							//}
							//if (action == 5) {
							//	robots[i]->takeShelfItem(tiles);
							//}
							//if (action == 6) {
							//	robots[i]->charge(tiles);
							//}
							//if (action >= 7 && action <= 8) {
							//	robots[i]->useElevator(tiles, action - 7);
							//}
							///*if (action >= 12 && action <= 51) {
							//	robots[i]->passItem(robots, tiles, (action - 12) % 10 + 1);
							//}*/
							//if (action == 9) {
							//	robots[i]->submitItems(tiles, itemList);
							//}

							robots[i]->sight(tiles, tileDatabase);

							/*delete(root);
							root = nullptr;*/
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
			for (int i = 0; i < MAX_ROBOTS; i++) {
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
	}

	// Delete robots
	for (int i = 0; i < MAX_ROBOTS; i++) {
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
		for (int i = 0; i < ITEMS_TO_RETRIEVE; i++) {
			if (itemList[i] == 0) itemsRetrieved++;
		}

		printf("Total Ticks: %d\n", ticks);
		if (itemsRetrieved > 0) printf("Average ticks per item: %f\n", (float)(ticks / itemsRetrieved));
		printf("Items retrieved: %d\n", itemsRetrieved);
	}
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

	return 0;
}