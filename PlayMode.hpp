#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct Obstacle {
	// Each obstacle is a 32x30 bitmask (each bit corresponds to one tile)
	// Bits 30-31 don't contain any information
	uint32_t mask[32];
};

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed ) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	uint8_t left_pressed, right_pressed, down_pressed, up_pressed, space_pressed = false;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f);

	//current x of left side of screen
	float scroll = 0.f;

	//library of all possible obstacles
	Obstacle obstacles[100];

	//tracks indices of current and next obstacles
	uint8_t cur_obstacle = 0;
	uint8_t next_obstacle = 0;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
