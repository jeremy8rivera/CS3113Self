#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include <SDL_mixer.h>
#include <algorithm>

#define MAX_BLOCKS 112
#define MAX_BULLETS 4



#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
enum GameState { MAIN, LEVEL, GAME_OVER };
enum Level { FIRST, SECOND, THIRD };
Level level;
GameState currState = MAIN;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

void drawText(ShaderProgram& program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> VertexData;
	std::vector<float> TexCoordData;

	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];

		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		VertexData.insert(VertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});

		TexCoordData.insert(TexCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}

	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, VertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, TexCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, VertexData.size() / 2);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);



}




class Bullet {
public:
	Bullet() {};
	Bullet(float posX, float posY, float velB) {
		x = posX;
		y = posY;
		width = 0.1f;
		height = 0.3f;
		velbase = velB;
	};
	float x;
	float y;
	float velbase;
	float velocity;
	float width;
	float height;
	float displacement;

	float checkDistance() {
		return 4.5f - abs(y);
	}

	void Draw(ShaderProgram *p) {
		float vertices[] = { x, y, x + width, y, x + width, y + height, x, y, x + width, y + height, x, y + height };
		glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(p->positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p->positionAttribute);
	}

	void movement(float elapsed) {
		velocity = elapsed * velbase;
		y += velocity;
	}

	//1 or -1 only

};

class Ball {
public:
	Ball() {};
	Ball(float posX, float posY, float velB, float xStart, float yStart) {
		angleX = cos(posX);
		angleY = sin(posY);
		x = xStart;
		y = yStart;
		width = 2.5f;
		height = 1.5f;
		velbase = velB;
	};

	float angleX;
	float angleY;
	float x;
	float y;
	float velbase;
	float velocity;
	float width;
	float height;

	void Draw(ShaderProgram *p) {
		float vertices[] = { x, y, x + width, y, x + width, y + height, x, y, x + width, y + height, x, y + height };
		glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(p->positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p->positionAttribute);
	}

	void movement(float elapsed, char val, Mix_Chunk *ballhit) {
		velocity = elapsed * velbase;
		if (val == 'x') {

			if (x + width >= 100.0f || x <= -100.0f) {
				angleX *= -1;
				Mix_PlayChannel(-1, ballhit, 0);
			}
			x += velocity*angleX;
		}
		else if (val == 'y') {

			if (y + height*2 >= 50.0f || y <= -50.0f) {
				angleY *= -1;
				Mix_PlayChannel(-1, ballhit, 0);

				if (y <= -50.0f) {
					currState = GAME_OVER;
				}
			}
			y += velocity*angleY;
		}
	}

};

class Entity {
public:
	Entity() {
		width = 8.0f;
		x = 0;
		y = 0;
		height = 2.5f;
	}
	Entity(float xPos, float yPos) : x(xPos), y(yPos) {
		width = 10.0f;
		height = 1.0f;
	};

	float x;
	float y;
	bool alive = 1;
	float width;
	float height;

	void Draw (ShaderProgram *p, GLuint& texture) {

		glBindTexture(GL_TEXTURE_2D, texture);
		float vertices[] = { x, y, x + width, y, x + width, y + height, x, y, x + width, y + height, x, y + height };
		glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(p->positionAttribute);


		float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(p->texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p->positionAttribute);
		glDisableVertexAttribArray(p->texCoordAttribute);
	};

	bool collisionDetection(Ball& ball) {
		if (((ball.x >= x && ball.x <= x + width) || (ball.x + ball.width >= x && ball.x + ball.width <= x + width)) && ((ball.y >= y && ball.y <= y + height) || (ball.y + ball.height <= y + height && ball.y + ball.height >= y))) {
			ball.velbase -= 0.2f;
			return true;
		}
		
		else {
			return false;
		}
	}

	void playerCollision(Ball &ball, Mix_Chunk *ballhit) {
		if (((ball.x >= x && ball.x <= x + width) || (ball.x + ball.width >= x && ball.x + ball.width <= x + width)) && ((ball.y >= y && ball.y <= y + height) || (ball.y + ball.height <= y + height && ball.y + ball.height >= y))) {
			if (ball.x  <= x + width/3 && ball.angleX < 0) {
				ball.angleX *= -1;
			}

			else if (ball.x >= x + width*2/3 && ball.angleX > 0) {
				ball.angleX *= -1;
			}
			ball.angleY *= -1;
			Mix_PlayChannel(-1, ballhit, 0);
		}
	}

};

bool removeBullet(Bullet bullet) {
	if (bullet.checkDistance() < 0.0f) {
		return true;
	}
	return false;
}

bool isDead(Entity entity) {
	if (entity.alive) {
		return false;
	}
	return true;
}


void renderGame(ShaderProgram& p, Entity& player1, Entity& player2, Ball& ball, std::vector<Entity>& blockVector, float elapsed, GLuint &brick1, GLuint& brick2, GLuint& playerTwoText) {
	player1.Draw(&p, brick2);
	player2.Draw(&p, playerTwoText);
	ball.Draw(&p);
	for (int i = 0; i < blockVector.size(); i++) {
		for (int i = 0; i < blockVector.size(); i++) {
			blockVector[i].Draw(&p, brick1);
		}
	}
}

void updateGame(ShaderProgram& p, Entity& player1, Entity& player2, Ball& ball, std::vector<Entity>& blockVector, float elapsed, Mix_Chunk *ballhit, Mix_Chunk *blockPoof) {
	ball.movement(elapsed, 'x', ballhit);

	if (blockVector.empty() == false) {
		for (int i = 0; i < blockVector.size(); i++) {
			if (blockVector[i].collisionDetection(ball)) {
				ball.angleX *= -1;
				blockVector[i].alive = 0;
				blockVector.erase(std::remove_if(blockVector.begin(), blockVector.end(), isDead), blockVector.end());
			}
		}
	}
	ball.movement(elapsed, 'y', ballhit);
	player1.playerCollision(ball, ballhit);
	player2.playerCollision(ball, ballhit);


	if (blockVector.empty() == false) {
		for (int i = 0; i < blockVector.size(); i++) {
			if (blockVector[i].collisionDetection(ball)) {
				Mix_PlayChannel(-1, blockPoof, 0);
				ball.angleY *= -1;
				blockVector[i].alive = 0;
				blockVector.erase(std::remove_if(blockVector.begin(), blockVector.end(), isDead), blockVector.end());
			}
		}
	}



}
void processInput(Entity& player1, Entity& player2, Ball& ball, std::vector<Entity>& blockVector, float elapsed, bool& done) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	switch (currState) {
	case MAIN:
		ball.velbase = 0.0f;

		if (keys[SDL_SCANCODE_1]) {
			blockVector.erase(blockVector.begin(), blockVector.end());
			Entity blocks[MAX_BLOCKS];
			ball.x = 0.0f;
			ball.y = player1.height + player1.y;
			ball.velbase = -30.0f;
			for (int i = 0; i < MAX_BLOCKS; i++) {

				if (i < MAX_BLOCKS / 8) {
					blocks[i].x = 0.0f - (70.0f - i * 10);
					blocks[i].y = 35.0f;
				}

				else if (i < MAX_BLOCKS * 2 / 8 && i >= MAX_BLOCKS / 8) {
					blocks[i].x = 0.0f - (70.0f - (i - (MAX_BLOCKS / 8)) * 10);
					blocks[i].y = 30.0f;
				}

				else if (i < MAX_BLOCKS * 3 / 8 && i >= MAX_BLOCKS * 2 / 8) {
					blocks[i].x = 0.0f - (70.0f - (i - (MAX_BLOCKS * 2 / 8)) * 10);
					blocks[i].y = 25.0f;
				}

				else if (i < MAX_BLOCKS * 4 / 8 && i >= MAX_BLOCKS * 3 / 8) {
					blocks[i].x = 0.0f - (70.0f - (i - (MAX_BLOCKS * 3 / 8)) * 10);
					blocks[i].y = 20.0f;
				}

				else if (i < MAX_BLOCKS * 5 / 8 && i >= MAX_BLOCKS * 4 / 8) {
					blocks[i].x = 0.0f - (70.0f - (i - (MAX_BLOCKS * 4 / 8)) * 10);
					blocks[i].y = 15.0f;
				}

				else if (i < MAX_BLOCKS * 6 / 8 && i >= MAX_BLOCKS * 5 / 8) {
					blocks[i].x = 0.0f - (70.0f - (i - (MAX_BLOCKS * 5 / 8)) * 10);
					blocks[i].y = 10.0f;
				}

				else if (i < MAX_BLOCKS * 7 / 8 && i >= MAX_BLOCKS * 6 / 8) {
					blocks[i].x = 0.0f - (70.0f - (i - (MAX_BLOCKS * 6 / 8)) * 10);
					blocks[i].y = 5.0f;
				}

				else if (i < MAX_BLOCKS * 8 / 8 && i >= MAX_BLOCKS * 7 / 8) {
					blocks[i].x = 0.0f - (70.0f - (i - (MAX_BLOCKS * 7 / 8)) * 10);
					blocks[i].y = 0.0f;
				}


				blockVector.push_back(blocks[i]);
			}


			level = FIRST;
			currState = LEVEL;
		}
		if (keys[SDL_SCANCODE_2]) {
			blockVector.erase(blockVector.begin(), blockVector.end());
			Entity blocks[MAX_BLOCKS + 5];
			ball.x = 0.0f;
			ball.y = player1.height + player1.y;
			ball.velbase = -30.0f;
			for (int i = 0; i < MAX_BLOCKS + 5; i++) {
				if (i < 12) {
					blocks[i].y = 40.0f - (4.0f * i);
					blocks[i].x = -90.0f;
				}

				if (i < 24 && i >= 12) {
					blocks[i].y = 40.0f - (4.0f * (i - 12));
					blocks[i].x = -60.0f;
				}

				if (i < 36 && i >= 24) {
					blocks[i].y = 40.0f - (4.0f * (i - 24));
					blocks[i].x = -30.0f;
				}

				if (i < 48 && i >= 36) {
					blocks[i].y = 40.0f - (4.0f * (i-36));
					blocks[i].x = -00.0f;
				}

				if (i < 60 && i >= 48) {
					blocks[i].y = 40.0f - (4.0f * (i - 48));
					blocks[i].x = 30.0f;
				}

				if (i < 72 && i >= 60) {
					blocks[i].y = 40.0f - (4.0f * (i - 60));
					blocks[i].x = 60.0f;
				}

				if (i < 84 && i >= 72) {
					blocks[i].y = 40.0f - (4.0f * (i - 72));
					blocks[i].x = 90.0f;
				}

				if (i < 96 && i >= 84) {
					blocks[i].y = 40.0f - (4.0f * (i - 84));
					blocks[i].x = 30.0f;
				}

				if (i < 117 && i >= 96) {
					blocks[i].y = -8.0f;
					blocks[i].x = -90.0f + (10.0f * (i - 96));
				}

				blockVector.push_back(blocks[i]);
			}
			
			level = SECOND;
			currState = LEVEL;
		}
		if (keys[SDL_SCANCODE_3]) {
			blockVector.erase(blockVector.begin(), blockVector.end());
			Entity blocks[MAX_BLOCKS];
			ball.x = 0.0f;
			ball.y = player1.height + player1.y;
			ball.velbase = -30.0f;

			for (int i = 0; i < MAX_BLOCKS; i++) {
				if (i < 15) {
					blocks[i].y = 40.0f - (4.0f * i);
					blocks[i].x = -90.0f + (10.0f * (i));
				}
				if (i < 30 && i >= 15) {
					blocks[i].y = -20.0f + (4.0f * (i-15));
					blocks[i].x = -90.0f + (10.0f * (i-15));
				}

				if (i < 45 && i >= 30) {
					blocks[i].y = -20.0f + (4.0f * (i - 30));
					blocks[i].x = -70.0f + (10.0f * (i - 30));
				}
				blockVector.push_back(blocks[i]);
			}

			level = THIRD;
			currState = LEVEL;
		}
		if (keys[SDL_SCANCODE_Q]) {
			done = true;
		}
		break;
	case LEVEL:
		if (keys[SDL_SCANCODE_LEFT] && player1.x >= -100.0f) {
			player1.x -= 75.0f * elapsed;
		}


		if (keys[SDL_SCANCODE_RIGHT] && player1.x + player1.width <= 100.0f) {
			player1.x += 75.0f * elapsed;

		}
		if (keys[SDL_SCANCODE_A] && player2.x >= -100.0f) {
			player2.x -= 75.0f * elapsed;
		}


		if (keys[SDL_SCANCODE_D] && player2.x + player2.width <= 100.0f) {
			player2.x += 75.0f * elapsed;

		}

		if (keys[SDL_SCANCODE_Q]) {
			done = true;
		}

		if (keys[SDL_SCANCODE_M]) {
			for (int i = 0; i < blockVector.size(); i++) {
					blockVector[i].alive = 0;
					blockVector.erase(blockVector.begin(), blockVector.end());
			}
			currState = MAIN;
		}
		break;
	case GAME_OVER:
		blockVector.erase(blockVector.begin(), blockVector.end());
		if (keys[SDL_SCANCODE_R] || keys[SDL_SCANCODE_M]) {
			currState = MAIN;
		}
		if (keys[SDL_SCANCODE_Q]) {
			done = true;
		}
		break;
	}


};

void renderTitle(ShaderProgram& p, GLuint& texture) {
	Matrix modelview;
	modelview.Identity();
	modelview.Translate(-40.0f, 10.0f, 0.0f);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Brick Breaker", 4.4, 1.2);

	modelview.Identity();
	modelview.Translate(-40.0f, 0.0f, 0.0f);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Q-quit", 3.2, 0.1);

	modelview.Identity();
	modelview.Translate(-40.0f, -5.0f, 0.0f);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "R,M-Menu", 3.2, 0.1);

	modelview.Identity();
	modelview.Translate(-40.0f, -10.0f, 0.0f);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "1,2,3-Levels", 3.2, 0.1);
	
}
void renderGameOver(ShaderProgram& p, GLuint & texture) {
	Matrix modelview;
	modelview.Identity();
	modelview.Translate(20.0f, 0.0f, 0.0f);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Game Over", 4.0, 1.2);
}
void renderGameWin(ShaderProgram& p, GLuint & texture) {

}


void Render(ShaderProgram& p, Entity& player1, Entity& player2, Ball& ball,
	std::vector<Entity>& blockVector, float elapsed, GLuint texture, GLuint& brick1, GLuint& brick2, GLuint& playerTwoText) {
	switch (currState) {
	case MAIN:
		renderTitle(p, texture);
		break;
	case LEVEL:
		renderGame(p, player1, player2, ball, blockVector, elapsed, brick1, brick2, playerTwoText);
		break;

	case GAME_OVER:
		renderGameOver(p, texture);
		break;
	}
}





int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif
	//Generate Viewport
	glViewport(0, 0, 960, 720);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint blockTexture = LoadTexture(RESOURCE_FOLDER"brick1.png");
	GLuint blockTexture2 = LoadTexture(RESOURCE_FOLDER"platform.png");
	GLuint playerTwoTexture = LoadTexture(RESOURCE_FOLDER"platform2.png");
	GLuint textText = LoadTexture(RESOURCE_FOLDER"font1.png");
	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-100.0f, 100.0f, -50.0f, 50.0f, -100.0f, 100.0f);
	Matrix modelviewMatrix;

	float lastframeticks = 0.0f;

	//Creation of Everything
	Entity playerOne(30.0f, -40.0f);
	Entity playerTwo(-30.0f, -40.0f);
	Ball playerBall(3.0f, 80.0f, -30.0f, 0.0f, playerOne.y + playerOne.height);
	std::vector<Bullet> playerBullet;
	std::vector<Entity> blockVector;
	std::vector<Bullet> enemyBullets;

	int bulletIndex = 0;

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	Mix_Chunk *ballhit;
	Mix_Chunk *blockPoof;
	ballhit = Mix_LoadWAV("Pickup_Coin5.wav");
	blockPoof = Mix_LoadWAV("Paste.wav");

	Mix_Music *music;
	music = Mix_LoadMUS("Space Cube.wav");

	Mix_PlayMusic(music, -1);


	int wallcounter = 0;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastframeticks;
		lastframeticks = ticks;

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.80f, 0.8f, 1.0f, 1.0f);
		glUseProgram(program.programID);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetModelviewMatrix(modelviewMatrix);

		processInput(playerOne, playerTwo, playerBall, blockVector, elapsed, done);
		Render(program, playerOne, playerTwo, playerBall, blockVector, elapsed, textText, blockTexture, blockTexture2, playerTwoTexture);
		updateGame(program, playerOne, playerTwo, playerBall, blockVector, elapsed, ballhit, blockPoof);
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
