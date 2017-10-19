#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include <vector>
#include <algorithm>

#define MAX_ENEMIES 13
#define MAX_BULLETS 4



#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

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


class Entity {
public:
	Entity() {
		width = 0.6f;
		x = 0;
		y = 0;
		height = 0.5;
	}
	Entity(float xPos, float yPos) : x(xPos), y(yPos) {
		width = 1.3f;
		height = 0.5f;
	};

	float x;
	float y;
	bool alive = 1;
	float width;
	float height;

	void Draw (ShaderProgram *p) {
		float vertices[] = { x, y, x + width, y, x + width, y + height, x, y, x + width, y + height, x, y + height };
		glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(p->positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p->positionAttribute);
	};


	bool collisionDetection(Bullet bullet) {
		if (((bullet.x >= x && bullet.x <= x + width) || (bullet.x + bullet.width >= x && bullet.x + bullet.width <= x + width)) && ((bullet.y >= y && bullet.y <= y + height) || (bullet.y + bullet.height <= y + height && bullet.y + bullet.height >= y))) {
			return true;
		}
		
		else {
			return false;
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


int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 480, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 480, 480);
	ShaderProgram program(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-10.0f, 10.0f, -5.0f, 5.0f, -10.0f, 10.0f);
	Matrix modelviewMatrix;

	float lastframeticks = 0.0f;

	Entity playerOne(0.0f, -4.8f);
	std::vector<Bullet> playerBullet;
	std::vector<Entity> enemyVector;
	std::vector<Bullet> enemyBullets;

	int bulletIndex = 0;
	Entity enemies[MAX_ENEMIES];
	for (int i = 0; i < MAX_ENEMIES; i++) {
		enemies[i].x = 0.0f - (8.0f- i);
		enemies[i].y = 4.2f;
		enemyVector.push_back(enemies[i]);
	}
	int wallcounter = 0;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastframeticks;
		lastframeticks = ticks;


		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program.programID);
		program.SetModelviewMatrix(modelviewMatrix);
		program.SetProjectionMatrix(projectionMatrix);


		//DRAW CLASSES
		playerOne.Draw(&program);
		for (int i = 0; i < enemyVector.size(); i++) {
			enemyVector[i].Draw(&program);
		}

		if (enemyBullets.size() > 0) {

			for (int i = 0; i < enemyBullets.size(); i++) {
				enemyBullets[i].Draw(&program);
				enemyBullets[i].movement(elapsed);

				if (removeBullet(enemyBullets[i]) == true) {
					bulletIndex--;
				}

				if (playerOne.collisionDetection(enemyBullets[i])) {
					playerOne.alive = 0;
				}

			}
			enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(), removeBullet), enemyBullets.end());

		}


		//PLAYERBULLET FUNCTIONS
		if (playerBullet.size() == 1) {
			playerBullet[0].Draw(&program);
			playerBullet[0].movement(elapsed);

			if (enemyVector.empty() == false) {
				for (int i = 0; i < enemyVector.size(); i++) {
					if (enemyVector[i].collisionDetection(playerBullet[0])) {
						enemyVector[i].alive = 0;
						enemyVector.erase(std::remove_if(enemyVector.begin(), enemyVector.end(), isDead), enemyVector.end());
						playerBullet[0].y = 11.0f;
					}
				}

			}
			playerBullet.erase(std::remove_if(playerBullet.begin(), playerBullet.end(), removeBullet), playerBullet.end());
		}




		//MOVEMENT
		if (keys[SDL_SCANCODE_LEFT] && playerOne.x >= -10.0f) {
			playerOne.x -= 10.0f * elapsed;
		}


		if (keys[SDL_SCANCODE_RIGHT] && playerOne.x + playerOne.width <= 10.0f) {
			playerOne.x += 10.0f * elapsed;

		}

		if (keys[SDL_SCANCODE_SPACE] && playerBullet.empty()) {
			Bullet myBullet((playerOne.x + playerOne.width / 2), -4.4f, 5.0f);
			playerBullet.push_back(myBullet);
		}

		//ENEMY MOVEMENT
		if (enemyVector.empty() == false) {
			if (enemyVector[0].y >= -3.8f) {
				if (enemyVector[enemyVector.size() - 1].x + enemyVector[enemyVector.size() - 1].width <= 10.0f && wallcounter % 2 == 0) {
					for (int i = 0; i < enemyVector.size(); i++) {
						enemyVector[i].x += 2.5f * elapsed * (wallcounter + 1) / 3;
					}
				}
				else if (enemyVector[enemyVector.size() - 1].x + enemyVector[enemyVector.size() - 1].width >= 10.0f && wallcounter % 2 == 0) {
					wallcounter++;
					for (int i = 0; i < enemyVector.size(); i++) {
						enemyVector[i].y -= 0.5;
					}
				}

				else if (enemyVector[0].x >= -10.0f && wallcounter % 2 == 1) {
					for (int i = 0; i < enemyVector.size(); i++) {
						enemyVector[i].x -= 2.5f * elapsed * (wallcounter + 1) / 3;
					}
				}

				else if (enemyVector[0].x <= -10.0f && wallcounter % 2 == 1) {
					wallcounter++;
					for (int i = 0; i < enemyVector.size(); i++) {
						enemyVector[i].y -= 0.5;
					}
				}

			}
		}


		//enemyshooting
		for (int i = 0; i < enemyVector.size(); i++) {
			int x = rand() % 9;
			if (x < 1 && bulletIndex < 4) {
				bulletIndex++;
				Bullet bullet(enemyVector[i].x + enemyVector[i].width / 2, enemyVector[i].y, -6.0f);
				enemyBullets.push_back(bullet);
			}
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
