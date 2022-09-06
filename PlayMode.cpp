#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	// TODO: load obstacles
	Obstacle test0;
	for (int i = 0; i < 32; i++) test0.mask[i] = 0;
	Obstacle test1;
	for (int i = 0; i < 32; i++) test1.mask[i] = 0xFFFFFFFF;
	Obstacle test2;
	for (int i = 0; i < 32; i++) test2.mask[i] = 0xF0F0F0F0;
	obstacles[0] = test0;
	obstacles[1] = test1;
	obstacles[2] = test2;

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
		float ScrollSpeed = 50.f;
		scroll += ScrollSpeed * elapsed;
	}

	// Adjust player position
	{
		float PlayerSpeed = space_pressed ? 300.f : 100.0f;

		int x_delta = 0;
		int y_delta = 0;
		if (left_pressed) x_delta--;
		if (right_pressed) x_delta++;
		if (down_pressed) y_delta--;
		if (up_pressed) y_delta++;

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
		uint32_t screen_width_px = PPU466::BackgroundWidth/2 * 8;
		if (uint32_t(scroll) >= screen_width_px) {
			scroll -= screen_width_px;
			player_at.x -= screen_width_px;
			cur_obstacle = next_obstacle;
			next_obstacle = std::rand()%3;
		}
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
