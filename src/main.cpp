#include <SFML/Graphics.hpp>

#include <cstddef>
#include <vector>

namespace
{
constexpr unsigned int WINDOW_SIZE = 1024;
constexpr int START_LEVEL = 3;
constexpr int MIN_LEVEL = 1;
constexpr int MAX_LEVEL = 10;
constexpr float MARGIN = 20.f;

// Build a rotation around an arbitrary point using SFML transforms.
sf::Transform rotationAround(const sf::Vector2f center, const sf::Angle angle)
{
	sf::Transform transform = sf::Transform::Identity;
	transform.translate(center);
	transform.rotate(angle);
	transform.translate({-center.x, -center.y});
	return transform;
}

void appendTransformed(std::vector<sf::Vector2f>& output,
                       const std::vector<sf::Vector2f>& input,
                       const sf::Transform& transform,
                       const bool reverse)
{
	if (reverse)
	{
		for (auto it = input.rbegin(); it != input.rend(); ++it)
			output.push_back(transform.transformPoint(*it));
	}
	else
	{
		for (const sf::Vector2f point : input)
			output.push_back(transform.transformPoint(point));
	}
}

// Generate the discrete Hilbert curve in integer grid coordinates.
// At level n there are 2^n points along each side of the grid, so the total number of vertices is 4^n.
std::vector<sf::Vector2f> generateHilbert(const int level)
{
	if (level == 1)
	{
		// Assignment base case: (0,0) -> (0,1) -> (1,1) -> (1,0)
		return {
			{0.f, 0.f},
			{0.f, 1.f},
			{1.f, 1.f},
			{1.f, 0.f}
		};
	}

	const int halfSize = 1 << (level - 1);
	const float center = (static_cast<float>(halfSize) - 1.f) * 0.5f;
	const std::vector<sf::Vector2f> previous = generateHilbert(level - 1);

	std::vector<sf::Vector2f> curve;
	curve.reserve(previous.size() * 4);

	// Lower-left: rotate the previous curve clockwise and reverse its traversal direction so the four sub-curves connect end-to-end.
	const sf::Transform lowerLeft =
		rotationAround({center, center}, sf::degrees(-90.f));
	appendTransformed(curve, previous, lowerLeft, true);

	// Upper-left: translated copy.
	sf::Transform upperLeft = sf::Transform::Identity;
	upperLeft.translate({0.f, static_cast<float>(halfSize)});
	appendTransformed(curve, previous, upperLeft, false);

	// Upper-right: translated copy.
	sf::Transform upperRight = sf::Transform::Identity;
	upperRight.translate(
		{static_cast<float>(halfSize), static_cast<float>(halfSize)});
	appendTransformed(curve, previous, upperRight, false);

	// Lower-right: rotate counter-clockwise, reverse the traversal, then translate the transformed copy into the lower-right quadrant.
	sf::Transform lowerRight = sf::Transform::Identity;
	lowerRight.translate({static_cast<float>(halfSize), 0.f});
	lowerRight.translate({center, center});
	lowerRight.rotate(sf::degrees(90.f));
	lowerRight.translate({-center, -center});
	appendTransformed(curve, previous, lowerRight, true);

	return curve;
}

sf::VertexArray buildCurve(const int level)
{
	const std::vector<sf::Vector2f> points = generateHilbert(level);

	sf::VertexArray curve(sf::PrimitiveType::LineStrip, points.size());
	for (std::size_t i = 0; i < points.size(); ++i)
	{
		curve[i].position = points[i];
		curve[i].color = sf::Color::White;
	}

	return curve;
}
} // namespace

int main()
{
	sf::RenderWindow window(
		sf::VideoMode({WINDOW_SIZE, WINDOW_SIZE}),
		"CPSC 453 HW1 - Hilbert Curve");

	int level = START_LEVEL;
	sf::VertexArray curve = buildCurve(level);

	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
			{
				window.close();
			}
			else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
			{
				if (keyPressed->code == sf::Keyboard::Key::Up && level < MAX_LEVEL)
				{
					++level;
					curve = buildCurve(level);
				}
				else if (keyPressed->code == sf::Keyboard::Key::Down && level > MIN_LEVEL)
				{
					--level;
					curve = buildCurve(level);
				}
			}
		}

		// Map grid coordinates to the 1024x1024 window. The negative y scale converts mathematical y-up coordinates to SFML screen space.
		const float drawableSize = static_cast<float>(WINDOW_SIZE) - 2.f * MARGIN;
		const float gridSpan = static_cast<float>((1u << level) - 1u);
		const float scale = drawableSize / gridSpan;

		sf::Transform screenTransform = sf::Transform::Identity;
		screenTransform.translate({MARGIN, MARGIN + drawableSize});
		screenTransform.scale({scale, -scale});

		sf::RenderStates states;
		states.transform = screenTransform;

		window.clear(sf::Color::Black);
		window.draw(curve, states);
		window.display();
	}
}
