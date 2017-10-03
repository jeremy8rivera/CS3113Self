#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"



#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

class Paddle {
public:
	Paddle();
	Paddle(float xPos, float yPos) : x(xPos), y(yPos) {
		width = 0.15f;
		height = 0.75f;
	};

	float x;
	float y;

	float width;
	float height;

	void Draw (ShaderProgram *p) {
		float vertices[] = { x, y, x + width, y, x + width, y + height, x, y, x + width, y + height, x, y + height };
		glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(p->positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p->positionAttribute);
	};

};

class Ball {
public:
	Ball() {};
	Ball(float posX, float posY, float velB) {
		angleX = cos(posX);
		angleY = sin(posY);
		x = -0.15;
		y = 0.05;
		width = 0.15f;
		height = 0.15f;
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

void movement(float elapsed) {
	velocity = elapsed * velbase;
	x += velocity*angleX;
	y += velocity*angleY;
}
};

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	Matrix modelviewMatrix;

	float lastframeticks = 0.0f;

	Paddle playerOne(3.3f, -0.25f);
	Paddle playerTwo(-3.45f, -0.25f);
	Ball ball(rand()%360, rand()%360, 0.0f);

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
		playerTwo.Draw(&program);
		ball.Draw(&program);


		//MOVEMENT
		if (keys[SDL_SCANCODE_DOWN] && playerOne.y >= -2.0f) {
			playerOne.y -= 3.0f * elapsed;
		}


		if (keys[SDL_SCANCODE_UP] && playerOne.y + playerOne.height <= 2.0f) {
			playerOne.y += 3.0f * elapsed;

		}

		if (keys[SDL_SCANCODE_W] && playerTwo.y + playerTwo.height <= 2.0f) {
			playerTwo.y += 2 * elapsed;
		}

		if (keys[SDL_SCANCODE_S] && playerTwo.y >= -2.0f) {
			playerTwo.y -= 2 * elapsed;
		}

		if (keys[SDL_SCANCODE_SPACE]) {
			ball.velbase = 3.5f;
			ball.x = -0.5f;
			ball.y = 0.25f;
			ball.angleX = cos(rand() % 360);
			ball.angleY = sin(rand() * 360);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			playerOne.y = -0.25f;
			playerTwo.y = -0.25f;
		}
		ball.movement(elapsed);

		//CollisionDetection
		if (playerOne.x < ball.x + ball.width && playerOne.y < ball.y + ball.height && playerOne.y + playerOne.height > ball.y) {
			ball.angleX = -ball.angleX;
		}

		if (playerTwo.x + playerTwo.width > ball.x && playerTwo.y < ball.y + ball.height && playerTwo.y + playerTwo.height > ball.y) {
			ball.angleX = -ball.angleX;
		}

		if (ball.y <= -2.0f) {
			ball.angleY = -ball.angleY;
		}
		if (ball.y + ball.height >= 2.0f) {
			ball.angleY = -ball.angleY;
		}

		if (ball.x + ball.width >= 3.55f) {
			ball.velbase = 0.0f;
			glClearColor(0.0f, 0.5f, 1.5f, 1.0f);

		}

		if (ball.x <= -3.55) {
			ball.velbase = 0.0f;
			glClearColor(0.0f, 1.0f, 0.3f, 1.0f);
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
