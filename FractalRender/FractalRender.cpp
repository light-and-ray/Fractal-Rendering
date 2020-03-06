#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <sstream>
using namespace sf;

int WIDTH = 1000;
int HEIGHT = 1000;

typedef Vector2<double> Vector2d;
typedef unsigned long long ull;

double g_inf = 8;
ull g_lim = 230;

Image PALETTE;

Color calcColor(Vector2d c, double& velAvg)
{
	Vector2d z0(0, 0), z1(0, 0);
	// (x+yi)^3 = x^3 - 3xy^2  +  (3x^2y - x^3)*i
	ull i = 0;
	velAvg = 0;
	double vel;
	double sn = 0;
	while (1)
	{
		z1.x = pow(z0.x, 3) - 3 * z0.x * pow(z0.y, 2) + c.x;
		z1.y = 3 * pow(z0.x, 2) * z0.y - pow(z0.y, 3) + c.y;
		
		sn = i - log(log(pow(z1.x, 2) + pow(z1.y, 2))/log(3))/log(3) +2;
		i += 1;

		vel = 1. / (1 + pow(z1.x - z0.x, 2) + pow(z1.y - z0.y, 2));
		//vel = (1. / (1 + pow(z0.x, 2) + pow(z0.y, 2)));
		velAvg += vel;

		z0 = z1;

		if (pow(z0.x, 2) + pow(z0.y, 2) >= g_inf)
		{
			//velAvg /= i;
			velAvg /= sn;
			return Color::White;
			//int col = (float)i / g_lim * 255;
			//return Color(col, col, col);
		}
		else if (i >= g_lim)
		{
			velAvg = -1;
			return Color(0, 0, 10);
		}
	}
}

void partRender(int startY, int stopY, Image& image, double start_x, double start_y, double step, int quality, double* velAvg)
{
	Vector2d c;
	c.x = start_x;
	c.y = start_y;
	Color color;
	for (int Y = startY; Y >= stopY ; Y -= quality)
	{
		for (int X = 0; X < WIDTH; X += quality)
		{
			color = calcColor(c, velAvg[X + Y * HEIGHT]);
			for (int i = 0; i < quality; i++)
			{
				if (X + i >= WIDTH) continue;
				for (int j = 0; j < quality; j++)
				{
					if (Y - j < 0) continue;
					image.setPixel(X+i, Y-j, color);
					velAvg[X + i + (Y - j) * HEIGHT] = velAvg[X + Y * HEIGHT];
				}
			}
			c.x += step * quality;
		}
		c.x = start_x;
		c.y += step * quality;
	}
}



void render(Image &image ,const double& start_x, const double& start_y, const double& size, int threads, int quality)
{
	Color color;

	double step = size / WIDTH;

	int STEP = HEIGHT / threads;

	double* velAvg_map = new double[HEIGHT * WIDTH];

	std::vector<std::thread> segment(threads);

	for (int i = 0; i < threads - 1; i++)
	{
		segment[i] = std::thread(
			partRender,
			HEIGHT - 1 - i * STEP,
			HEIGHT - (i + 1) * STEP,
			std::ref(image),
			start_x,
			start_y + i * step * STEP,
			step,
			quality,
			velAvg_map);
	}
	segment[threads - 1] = std::thread(
		partRender,
		HEIGHT - 1 - (threads - 1) * STEP,
		0,
		std::ref(image),
		start_x,
		start_y + (threads-1) * step * STEP,
		step,
		quality,
		velAvg_map);

	for (int i = 0; i < threads; i++)
	{
		segment[i].join();	
	}

	{ //paint exterier
		double map_min = -1;
		double map_max = -1;

		for (int i = 1; i < WIDTH * HEIGHT; i++)
		{
			if (velAvg_map[i] == -1) continue;
			if (velAvg_map[i] < map_min or map_min == -1) map_min = velAvg_map[i];
			if (velAvg_map[i] > map_max or map_max == -1) map_max = velAvg_map[i];
		}

		Color col;

		for (int y = 0; y < HEIGHT; y++)
		{
			for (int x = 0; x < WIDTH; x++)
			{
				if (velAvg_map[x + y * HEIGHT] == -1) continue;				
				col = PALETTE.getPixel(
					(velAvg_map[x + y * HEIGHT] - map_min) / (map_max - map_min) * PALETTE.getSize().x,
					PALETTE.getSize().y / 2);
				image.setPixel(x, y, col);
			}
		}
	}

	delete[] velAvg_map;
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
		Image image;
		image.create(WIDTH, HEIGHT);
		render(image, x, y, size, threads, quality);
		texture.loadFromImage(image);
		RectangleShape rect_;
		rect_.setTexture(&texture);
		rect_.setPosition(0, 0);
		rect_.setSize(Vector2f(WIDTH, HEIGHT));
		rect = rect_;
		window->clear();
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
		if (lastRenderClock.getElapsedTime().asMilliseconds() >= 200 * (3-renderingProgres))
		{
			switch (renderingProgres)
			{
			case 2:
				std::cout 
					<< "\rc = " << x + size/2 << "+" << y + size /2 << "i | "
					<< "lim = " << g_lim << " size = " << size
					<< "             ";
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
		int controlTimeLimit = 200;
		int frames = 30;
		if (clockControl.getElapsedTime().asMilliseconds() >= controlTimeLimit and
			window->hasFocus())
		{
			//move
			{
				if (Keyboard::isKeyPressed(Keyboard::Up) or
					Keyboard::isKeyPressed(Keyboard::W))
				{
					y += (!Keyboard::isKeyPressed(Keyboard::LAlt)) ? size / 5 : size / 25;
					needDisplay = true;
					renderingProgres = 2;
					//lastRenderClock.restart();
					clockControl.restart();
				}

				if (Keyboard::isKeyPressed(Keyboard::Down) or
					Keyboard::isKeyPressed(Keyboard::S))
				{
					y -= (!Keyboard::isKeyPressed(Keyboard::LAlt)) ? size / 5 : size / 25;
					needDisplay = true;
					renderingProgres = 2;
					//lastRenderClock.restart();
					clockControl.restart();
				}

				if (Keyboard::isKeyPressed(Keyboard::Left) or
					Keyboard::isKeyPressed(Keyboard::A))
				{
					x -= (!Keyboard::isKeyPressed(Keyboard::LAlt)) ? size / 5 : size / 25;
					needDisplay = true;
					renderingProgres = 2;
					//lastRenderClock.restart();
					clockControl.restart();
				}

				if (Keyboard::isKeyPressed(Keyboard::Right) or
					Keyboard::isKeyPressed(Keyboard::D))
				{
					x += (!Keyboard::isKeyPressed(Keyboard::LAlt)) ? size / 5 : size / 25;
					needDisplay = true;
					renderingProgres = 2;
					//lastRenderClock.restart();
					clockControl.restart();
				}
			}

			//zoom
			{
			if (Keyboard::isKeyPressed(Keyboard::Equal))
			{
				double zoom = (!Keyboard::isKeyPressed(Keyboard::LAlt)) ? 1.5 : 1.1;
				x += (size - size / zoom) / 2;
				y += (size - size / zoom) / 2;
				size /= zoom;
				double zoom_ = pow(zoom, 1. / frames);
				for (int i = 0; i < frames; i++)
				{
					rect.move(-rect.getSize().x * rect.getScale().x * (zoom_ / 2 - 1. / 2),
						-rect.getSize().y * rect.getScale().y * (zoom_ / 2 - 1. / 2));
					rect.scale(zoom_, zoom_);
					window->draw(rect);
					window->display();

					Clock clck;
					while (clck.getElapsedTime().asMilliseconds() < controlTimeLimit / frames) {}
				}


				needDisplay = true;
				renderingProgres = 2;
				lastRenderClock.restart();
				clockControl.restart();
			}

			if (Keyboard::isKeyPressed(Keyboard::Dash))
			{
				double zoom = (!Keyboard::isKeyPressed(Keyboard::LAlt)) ? 1.5 : 1.1;
				size *= 1.5;
				x -= (size - size / zoom) / 2;
				y -= (size - size / zoom) / 2;
				needDisplay = true;
				renderingProgres = 2;
				//lastRenderClock.restart();
				clockControl.restart();
			}
			}


			if (Keyboard::isKeyPressed(Keyboard::F2))
			{
				g_lim /= 1.1;
				clockControl.restart();
			}
			if (Keyboard::isKeyPressed(Keyboard::F3))
			{
				g_lim *= 1.1;
				needDisplay = true;
				renderingProgres = 2;
				clockControl.restart();
			}

			if (Keyboard::isKeyPressed(Keyboard::F4))
			{
				std::stringstream name;
				name << "shot_" << time(NULL) << "_re" << x + size/2 << "_im" << y + size / 2 << "_size" << size << "lim" << g_lim << ".png";
				texture.copyToImage().saveToFile(name.str());
				std::cout << "\n" << name.str() << " SAVED\n";
				clockControl.restart();
			}
		}
	}

public:
	Fractal(RenderWindow* window, double x = -1.5, double y = -1.5, double size = 3, int threads = 21) :
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

	void onResize(float scale)
	{
		needDisplay = true;
		renderingProgres = 2;
	}
};

int main()
{
	if (!PALETTE.loadFromFile("palette.png"))
	{
		std::cout << "error: faild palette.png load\n\n";
		Clock clk;
		while (clk.getElapsedTime().asSeconds() < 3) {}
		exit(1);
	}

	std::cout << "author: light-and-ra\n"
		<< "press \+ or \- to zoom\n"
		<< "wasd or cross to move\n"
		<< "hold LAlt to slow\n"
		<< "F2 - decrease iterations limit\n"
		<< "F3 - increase iterations limit\n"
		<< "F4 to take shot\n";

	std::cout << "\n";

	RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "FractalRender");
	Image icon;
	icon.loadFromFile("icon.png");
	window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
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

			if (event.type == Event::Resized)
			{
				window.setSize(Vector2u(WIDTH, HEIGHT));
			}
		}

		fractal.procces();
	}
}