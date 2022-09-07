#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include "data_path.hpp"

#include <fstream>

#include <random>

#define TILE_SIZE 8

PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	// Loading in the actual obstacles
	std::ifstream fin (data_path("data/processed_obstacles.dat"));
	fin >> num_obstacles;
	for (int i = 0; i < num_obstacles; i++) {
		for (int j = 0; j < 32; j++) {
			uint32_t row = 0;
			for (int k = 0; k < 30; k++) {
				int x; fin >> x;
				row <<= 1;
				row += x;
			}
			obstacles[i].mask[j] = row;
		}
	}

	//used for obstacle (filled):
	ppu.palette_table[0] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};
	ppu.tile_table[0].bit0 = {0, 0, 0, 0, 0, 0, 0, 0};
	ppu.tile_table[0].bit1 = {0, 0, 0, 0, 0, 0, 0, 0};

	//used for obstacle (empty):
	ppu.palette_table[1] = {
		glm::u8vec4(0xff, 0xff, 0xff, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};
	ppu.tile_table[1].bit0 = {0, 0, 0, 0, 0, 0, 0, 0};
	ppu.tile_table[1].bit1 = {0, 0, 0, 0, 0, 0, 0, 0};

	//use sprite 32 as a "player":
	ppu.tile_table[32].bit0 = {
		0b01111110,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b01111110,
	};
	ppu.tile_table[32].bit1 = {
		0b00000000,
		0b00000000,
		0b00011000,
		0b00100100,
		0b00000000,
		0b00100100,
		0b00000000,
		0b00000000,
	};

	//used for the player:
	ppu.palette_table[7] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0xff, 0xff, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//set starting values of scroll and player position
	uint32_t screen_width_px = PPU466::BackgroundWidth/2 * TILE_SIZE;
	uint32_t screen_height_px = PPU466::BackgroundHeight/2 * TILE_SIZE;
	scroll = float(screen_width_px);
	player_at.x = 15 * float(screen_width_px) / 8;
	player_at.y = float(screen_height_px / 2);

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left_pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right_pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up_pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down_pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space_pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left_pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right_pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up_pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down_pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space_pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	// Adjust scrolling
	{
		tot_elapsed += elapsed;
		float ScrollSpeed = tot_elapsed < 1 ? 20.f : 80.f;
		scroll += ScrollSpeed * elapsed;
	}

	// Adjust player position
	{
		float PlayerSpeed = space_pressed ? 200.f : 100.0f;

		int x_delta = 0;
		int y_delta = 0;
		if (left_pressed) {
			x_delta--;
		}
		if (right_pressed) {
			x_delta++;
		}
		if (down_pressed) {
			y_delta--;
		}
		if (up_pressed) {
			y_delta++;
		}

		// Make diagonal movement same speed
		static float sqrt2_recip = 1.f / sqrtf(2.f);
		if (x_delta && y_delta) PlayerSpeed *= sqrt2_recip;

		if (left_pressed) player_at.x -= PlayerSpeed * elapsed;
		if (right_pressed) player_at.x += PlayerSpeed * elapsed;
		if (down_pressed) player_at.y -= PlayerSpeed * elapsed;
		if (up_pressed) player_at.y += PlayerSpeed * elapsed;
	}

	// Update obstacles
	{
		// if we just scrolled to a new obstacle:
		uint32_t screen_width_px = PPU466::BackgroundWidth/2 * TILE_SIZE;
		if (uint32_t(scroll) >= screen_width_px) {
			scroll -= screen_width_px;
			player_at.x -= screen_width_px;
			cur_obstacle = next_obstacle;
			next_obstacle = 1 + std::rand()%(num_obstacles - 1);
		}
		// draw the current and next obstacles on the background
		// TODO: optimize so this doesn't happen every iteration
		for (uint32_t y = 0; y < PPU466::BackgroundHeight/2; ++y) {
			for (uint32_t x = 0; x < PPU466::BackgroundWidth/2; ++x) {
				bool is_filled = obstacles[cur_obstacle].mask[x] & (1 << y);
				uint16_t tile_mask;
				if (is_filled) {
					tile_mask = 0;
				} else {
					tile_mask = 0x101;
				}
				ppu.background[x+PPU466::BackgroundWidth*y] = tile_mask;
			}
		}
		for (uint32_t y = 0; y < PPU466::BackgroundHeight/2; ++y) {
			for (uint32_t x = 0; x < PPU466::BackgroundWidth/2; ++x) {
				bool is_filled = obstacles[next_obstacle].mask[x] & (1 << y);
				uint16_t tile_mask;
				if (is_filled) {
					tile_mask = 0;
				} else {
					tile_mask = 0x101;
				}
				ppu.background[PPU466::BackgroundWidth/2 + x+PPU466::BackgroundWidth*y] = tile_mask;
			}
		}
	}

	// Check for collisions
	{
		uint32_t screen_width_px = PPU466::BackgroundWidth/2 * TILE_SIZE;
		uint32_t screen_height_px = PPU466::BackgroundHeight/2 * TILE_SIZE;
		
		bool collided = false;

		// Check if player is on the screen
		float sx = player_at.x - scroll;
		float sy = player_at.y;
		if (sx < 0 || sx + TILE_SIZE - 1 >= screen_width_px || sy < 0 || sy + TILE_SIZE - 1 >= screen_height_px) {
			collided = true;
		}

		// Function to check if a given tile is in bounds, filled, and intersects with the player
		auto collides = [&](int x, int y) {
			// Check that this tile is in bounds
			if (x < 0 || y < 0 || x >= PPU466::BackgroundWidth || y >= PPU466::BackgroundHeight) return false;

			// Check that this tile is filled
			// Case 1: we are in cur_obstacle
			if (x < PPU466::BackgroundWidth/2) {
				bool is_filled = obstacles[cur_obstacle].mask[x] & (1 << y);
				if (!is_filled) return false;
			}
			// Case 2: we are in next_obstacle
			else {
				bool is_filled = obstacles[next_obstacle].mask[x - PPU466::BackgroundWidth/2] & (1 << y);
				if (!is_filled) return false;
			}

			// Check if this tile intersects with the player's position
			bool x_intersects = abs(player_at.x - x*8) < 8;
			bool y_intersects = abs(player_at.y - y*8) < 8;
			return x_intersects && y_intersects;
		};

		// Need to check the tiles in the 3x3 square surrounding (tx, ty)
		int tx = int(player_at.x / TILE_SIZE);
		int ty = int(player_at.y / TILE_SIZE);
		for (int x = tx-1; x <= tx+1; x++) {
			for (int y = ty-1; y <= ty+1; y++) {
				collided |= collides(x, y);
			}
		}
		assert(!collided);
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//background position:
	ppu.background_position.x = int32_t(-scroll);
	
	//player sprite:
	ppu.sprites[0].x = int8_t(player_at.x - scroll);
	ppu.sprites[0].y = int8_t(player_at.y);
	ppu.sprites[0].index = 32;
	ppu.sprites[0].attributes = 7;

	//--- actually draw ---
	ppu.draw(drawable_size);
}
