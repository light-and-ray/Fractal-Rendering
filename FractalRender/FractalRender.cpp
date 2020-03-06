#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <thread>
using namespace sf;

constexpr int WIDTH = 900;
constexpr int HEIGHT = 900;

typedef Vector2<double> Vector2d;
typedef unsigned long long ull;

double g_inf = 100;
ull g_lim = 100;

Color calcColor(Vector2d c)
{
	Vector2d z0(0, 0), z1(0, 0);
	// (x+yi)^3 = x^3 - 3xy^2  +  (3x^2y - x^3)*i
	ull i = 0;
	while (1)
	{
		z1.x = pow(z0.x, 3) - 3 * z0.x * pow(z0.y, 2) + c.x;
		z1.y = 3 * pow(z0.x, 2) * z0.y - pow(z0.y, 3) + c.y;
		z0 = z1;
		i++;

		if (pow(z0.x, 2) + pow(z0.y, 2) >= g_inf)
		{
			//return Color::White;
			int col = (float)i / g_lim * 255;
			return Color(col, col, col);
		}
		else if (i >= g_lim)
		{
			return Color::Black;
		}
	}
}

void partRender(int startY, int stopY, Image& image, double start_x, double start_y, double step, int quality)
{
	Vector2d c;
	c.x = start_x;
	c.y = start_y;
	Color color;
	for (int Y = startY; Y >= stopY ; Y -= quality)
	{
		for (int X = 0; X < WIDTH; X += quality)
		{
			color = calcColor(c);
			for (int i = 0; i < quality; i++)
			{
				if (X + i >= WIDTH) continue;
				for (int j = 0; j < quality; j++)
				{
					if (Y - j < 0) continue;
					image.setPixel(X+i, Y-j, color);
				}
			}
			c.x += step * quality;
		}
		c.x = start_x;
		c.y -= step * quality;
	}
}

Texture render(const double& start_x, const double& start_y, const double& size, int threads, int quality)
{
	Image image;
	Color color;

	image.create(WIDTH, HEIGHT);

	double step = size / WIDTH;

	int STEP = HEIGHT / threads;

	std::vector<std::thread> segment(threads);
	for (int i = 0; i < threads - 1; i++)
	{
		segment[i] = std::thread(
			partRender,
			HEIGHT - 1 - i * STEP,
			HEIGHT - (i + 1) * STEP,
			std::ref(image),
			start_x,
			start_y - i * step * STEP,
			step,
			quality);
	}
	segment[threads - 1] = std::thread(
		partRender,
		HEIGHT - 1 - (threads-1) * STEP,
		0,
		std::ref(image),
		start_x,
		start_y - (threads-1) * step * STEP,
		step,
		quality);

	for (int i = 0; i < threads; i++)
	{
		segment[i].join();
	}
	
	Texture texture;
	texture.loadFromImage(image);
	return  texture;
}



class Fractal
{
	double x, y, size;
	int threads;
	
	Texture texture;
	RectangleShape rect;

	bool needDisplay = true;
	Clock lastRenderClock;
	Clock clockControl;

	int renderingProgres = 2;

	RenderWindow* window;

	void draw(int quality)
	{
		window->clear();
		texture = render(x, y, size, threads, quality);
		rect.setTexture(&texture);
		rect.setSize(Vector2f(WIDTH, HEIGHT));
		rect.setPosition(0, 0);
		window->draw(rect);
		window->display();
	}

	void display()
	{
		if (renderingProgres == -1)
		{
			needDisplay = false;
			return;
		}
		if (lastRenderClock.getElapsedTime().asMilliseconds() >= 500 * (3-renderingProgres))
		{
			switch (renderingProgres)
			{
			case 2:
			case 1:
			case 0:
				draw(pow(2, renderingProgres));
				renderingProgres -= 1;
				lastRenderClock.restart();
				break;
			default:
				break;
			}
		}
	}

	void control()
	{
		if (clockControl.getElapsedTime().asMilliseconds() >= 200 and 
			window->hasFocus())
		{
			if (Keyboard::isKeyPressed(Keyboard::Up) or
				Keyboard::isKeyPressed(Keyboard::W))
			{
				y -= size / 5;
				needDisplay = true;
				renderingProgres = 2;
				//lastRenderClock.restart();
				clockControl.restart();
			}

			if (Keyboard::isKeyPressed(Keyboard::Down) or
				Keyboard::isKeyPressed(Keyboard::S))
			{
				y += size / 5;
				needDisplay = true;
				renderingProgres = 2;
				//lastRenderClock.restart();
				clockControl.restart();
			}

			if (Keyboard::isKeyPressed(Keyboard::Left) or
				Keyboard::isKeyPressed(Keyboard::A))
			{
				x -= size / 5;
				needDisplay = true;
				renderingProgres = 2;
				//lastRenderClock.restart();
				clockControl.restart();
			}

			if (Keyboard::isKeyPressed(Keyboard::Right) or
				Keyboard::isKeyPressed(Keyboard::D))
			{
				x += size / 5;
				needDisplay = true;
				renderingProgres = 2;
				//lastRenderClock.restart();
				clockControl.restart();
			}

			if (Keyboard::isKeyPressed(Keyboard::Equal))
			{
				x += 1. / 6 * size;
				y -= 1. / 6 * size;
				size /= 1.5;
				//rect.scale(1.5, 1.5);
				//rect.move(-0.5 * WIDTH, -0.5 * HEIGHT);
				window->draw(rect);
				window->display();
				needDisplay = true;
				renderingProgres = 2;
				//lastRenderClock.restart();
				clockControl.restart();
			}

			if (Keyboard::isKeyPressed(Keyboard::Dash))
			{
				size *= 1.5;
				x -= 1. / 6 * size;
				y += 1. / 6 * size;
				needDisplay = true;
				renderingProgres = 2;
				//lastRenderClock.restart();
				clockControl.restart();
			}

			if (Keyboard::isKeyPressed(Keyboard::F1))
			{
				g_lim /= 1.1;
				clockControl.restart();
			}
			if (Keyboard::isKeyPressed(Keyboard::F2))
			{
				g_lim *= 1.1;
				needDisplay = true;
				renderingProgres = 2;
				clockControl.restart();
			}
		}
	}

public:
	Fractal(RenderWindow* window, double x = -1.5, double y = 1.5, double size = 3, int threads = 21) :
		x(x), y(y), size(size), threads(threads), window(window) 
	{
		rect.setSize(Vector2f(WIDTH, HEIGHT));
		rect.setPosition(0, 0);
	};

	void procces()
	{
		if (needDisplay)
		{
			display();
		}
		control();
	}
};

int main()
{
	std::cout << "author: light-and-ra\n"
		<< "press \+ or \- to zoom\n"
		<< "wasd or cross to move\n"
		<< "F1 - decrease iterations limit\n"
		<< "F2 - increase iterations limit\n";
	RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "FractalRender");
	Fractal fractal(&window);

	while (window.isOpen())
	{
		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed or
				Keyboard::isKeyPressed(Keyboard::Escape))
			{
				window.close();
				exit(0);
			}
		}

		fractal.procces();
	}
}