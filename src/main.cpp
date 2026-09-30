#include <iostream>
#include <optional>
#include <vector>

#include <SFML/Graphics.hpp>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int FPS_LIMIT = 30;

using Point2D = sf::Vector2f;

// Define a function that samples a cubic Bezier curve at t in [0, 1].
Point2D getPoint(const std::vector<sf::Vector2f>& pts_list, float t) { 
    if (pts_list.size() <= 0) {
        return {0, 0};
    }
    float point_x = ((1 - t) * (1 - t) * (1 - t) * pts_list.at(0).x) +
                    (3 * (1 - t) * (1 - t) * t * pts_list.at(1).x) +
                    (3 * (1 - t) * t * t * pts_list.at(2).x) + (t * t * t * pts_list.at(3).x);
    float point_y = ((1 - t) * (1 - t) * (1 - t) * pts_list.at(0).y) +
                    (3 * (1 - t) * (1 - t) * t * pts_list.at(1).y) +
                    (3 * (1 - t) * t * t * pts_list.at(2).y) + (t * t * t * pts_list.at(3).y);
    //Formula taken from https://en.wikipedia.org/wiki/Bézier_curve
    return Point2D{point_x, point_y}; 
}

// Define a function that returns the curve's slope at t in [0, 1].
Point2D getSlope(const std::vector<sf::Vector2f>& pts_list, float t) { 
    if (pts_list.size() <= 0) {
        return {0, 0};
    }
    float point_x = (3 * (1 - t) * (1 - t) * (pts_list.at(1).x - pts_list.at(0).x)) +
                    (6 * (1 - t) * t * (pts_list.at(2).x - pts_list.at(1).x)) +
                    (3 * t * t * (pts_list.at(3).x - pts_list.at(2).x));
    float point_y = (3 * (1 - t) * (1 - t) * (pts_list.at(1).y - pts_list.at(0).y)) +
                    (6 * (1 - t) * t * (pts_list.at(2).y - pts_list.at(1).y)) +
                    (3 * t * t * (pts_list.at(3).y - pts_list.at(2).y));
    //Formula taken from https://probablymarcus.com/blocks/2015/02/26/using-bezier-curves-as-easing-functions.html
    return Point2D{point_x, point_y}; 
}

// Store four control points for the curve.
std::vector<sf::Vector2f> pts = {{125, 54}, {504, 187}, {353.5, 373}, {160, 317}};
// Track animation time for the square moving along the curve.
float FRAME = 0;
// Track the index of the control point being dragged.
Point2D starting_position;
bool dragging = false;
int control_index;

float Distance(Point2D from, Point2D to) {
    return sqrt(((to.x - from.x) * (to.x - from.x)) + ((to.y - from.y) * (to.y - from.y)));
}

void handleInput(sf::Window& window, bool& shouldQuit) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            shouldQuit = true;
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
            // On left-click, select the closest control point
            // using mouse->position and start dragging it.
            if (mouse->button == sf::Mouse::Button::Left) {
                starting_position = Point2D(mouse->position.x, mouse->position.y);
                Point2D closest_point = *pts.begin();
                for (Point2D point : pts) {
                    //std::cout << point.x << std::endl;
                    //std::cout << point.y << std::endl;
                    float new_dist =
                        sqrt(((point.x - mouse->position.x) * (point.x - mouse->position.x)) +
                              ((point.y - mouse->position.y) * (point.y - mouse->position.y)));
                    float old_dist =
                        sqrt(((closest_point.x - mouse->position.x) *
                              (closest_point.x - mouse->position.x)) +
                                          ((closest_point.y - mouse->position.y) *
                                           (closest_point.y - mouse->position.y)));
                    if (new_dist < old_dist) {
                        closest_point = point;
                    }
                }
                auto dragged_point = find(pts.begin(), pts.end(), closest_point);
                control_index = distance(pts.begin(), dragged_point);
                //pts.at(control_index).x = mouse->position.x;
                //pts.at(control_index).y = mouse->position.y;
                dragging = true;
            }
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonReleased>()) {
            // On left-button release, stop dragging.
            dragging = false;
        } else if (const auto* mouse = event->getIf<sf::Event::MouseMoved>()) {
            // Move the selected control point to mouse->position.
            if (dragging) {
                pts.at(control_index).x = mouse->position.x;
                pts.at(control_index).y = mouse->position.y;
                // Maintain matching slopes at shared endpoints.
                // When moving point 3, move point 5 without changing its distance
                // from point 4 (point numbers here start at 1).
                if ((control_index + 2 < pts.size()) && ((control_index + 1) % 3 == 0)) {  // control_index = 2
                    float self_mid_dist = Distance(pts.at(control_index), pts.at(control_index + 1));
                    float mid_other_dist = Distance(pts.at(control_index + 1), pts.at(control_index + 2));
                    pts.at(control_index + 2).x =
                        pts.at(control_index + 1).x + ((pts.at(control_index + 1).x - pts.at(control_index).x) / self_mid_dist * mid_other_dist);
                    pts.at(control_index + 2).y =
                        pts.at(control_index + 1).y + ((pts.at(control_index + 1).y - pts.at(control_index).y) / self_mid_dist * mid_other_dist);
                }
                if (!(control_index < 3) && ((control_index - 1) % 3 == 0)) {  // control_index = 4
                    float self_mid_dist =
                        Distance(pts.at(control_index), pts.at(control_index - 1));
                    float mid_other_dist =
                        Distance(pts.at(control_index - 1), pts.at(control_index - 2));
                    pts.at(control_index - 2).x =
                        pts.at(control_index - 1).x -
                        ((pts.at(control_index).x - pts.at(control_index - 1).x) / self_mid_dist *
                         mid_other_dist);
                    pts.at(control_index - 2).y =
                        pts.at(control_index - 1).y -
                        ((pts.at(control_index).y - pts.at(control_index - 1).y) / self_mid_dist *
                         mid_other_dist);
                } 
            }
            
        } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            // '+' adds three control points; '-' removes three,
            // keeping at least four points.
            if (key->scancode == sf::Keyboard::Scan::Equal ||
                key->scancode == sf::Keyboard::Scan::NumpadPlus) {
                Point2D endslope = getSlope(pts, 1);
                //std::cout << endslope.x << std::endl;
                //std::cout << endslope.y << std::endl;
                float x_diff =
                    (*(pts.begin() + pts.size() - 1)).x - (*(pts.begin() + pts.size() - 2)).x;
                float y_diff =
                    (*(pts.begin() + pts.size() - 1)).y - (*(pts.begin() + pts.size() - 2)).y;
                pts.push_back(Point2D({pts.back().x + x_diff, pts.back().y + y_diff}));
                pts.push_back(Point2D({600, 400}));
                pts.push_back(Point2D({400, 600}));
            } else if (key->scancode == sf::Keyboard::Scan::Hyphen ||
                       key->scancode == sf::Keyboard::Scan::NumpadMinus) {
                if ((pts.size() - 3) >= 4) {
                    pts.pop_back();
                    pts.pop_back();
                    pts.pop_back();
                }

            }
        }
    }
}

// Copied DrawLine function from Project 1a, modified for lab to calculate length within function and to return sf::ConvexShape instead. 
sf::ConvexShape DrawLine(Point2D from, Point2D to, float width, sf::Color c) {
    float length = Distance(from, to);
    float fromx_1 = from.x - (width / 2) * ((to.y - from.y) / length);
    float fromy_1 = from.y + (width / 2) * ((to.x - from.x) / length);
    float fromx_2 = from.x + (width / 2) * ((to.y - from.y) / length);
    float fromy_2 = from.y - (width / 2) * ((to.x - from.x) / length);
    float tox_1 = to.x - (width / 2) * ((to.y - from.y) / length);
    float toy_1 = to.y + (width / 2) * ((to.x - from.x) / length);
    float tox_2 = to.x + (width / 2) * ((to.y - from.y) / length);
    float toy_2 = to.y - (width / 2) * ((to.x - from.x) / length);
    sf::ConvexShape line;
    line.setPointCount(4);
    line.setPoint(0, {fromx_1, fromy_1});
    line.setPoint(1, {tox_1, toy_1});
    line.setPoint(2, {tox_2, toy_2});
    line.setPoint(3, {fromx_2, fromy_2});
    line.setFillColor(c);
    return line;
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);
    // ====== ====== ======
    // Sample GetPoint over t in [0, 1] and connect samples using the line-drawing
    // code from your project. Draw all four control points as circles after drawing the curve.
    // ====== ====== ======
    for (float i = 0; i < 1; i += 0.002) {
        //std::cout << "line drawn" << std::endl;
        window.draw(DrawLine(getPoint(pts, i), getPoint(pts, i + 0.001), 1, sf::Color::Cyan));
    }
    for (Point2D point: pts) {
        sf::CircleShape circle;
        circle.setRadius(10);
        circle.setFillColor(sf::Color::Red);
        circle.setOrigin({10, 10});
        circle.setPosition({point.x, point.y});
        window.draw(circle);
    }

    

    // ====== ====== ======
    // Draw a small square moving repeatedly along the curve.
    // Use GetSlope to orient it to the curve at each time step.
    // ====== ====== ======
    sf::RectangleShape movingsquare;
    movingsquare.setSize({10, 10});
    movingsquare.setOrigin({5, 5});
    movingsquare.setPosition(getPoint(pts, FRAME));
    movingsquare.setRotation(
        sf::radians(std::atan(getSlope(pts, FRAME).y / getSlope(pts, FRAME).x)));
    window.draw(movingsquare);


    // ====== ====== ======
    // Draw control handles from point 1 to 2 and point 3 to 4.
    window.draw(DrawLine(pts.at(0), pts.at(1), 2, sf::Color::White));
    window.draw(DrawLine(pts.at(2), pts.at(3), 2, sf::Color::White));
    // Draw all connected cubic Bezier segments and their handles.
    // ====== ====== ======
    if (pts.size() > 4) {
        int iterated = 0;
        for (int j = pts.size() - 4; j > 0; j -= 3) {
            std::vector<sf::Vector2f> templist(pts.begin() + 3 + (3 * iterated),
                                               pts.begin() + 3 + (3 * iterated) + 4);
            for (float i = 0; i < 1; i += 0.002) {
                window.draw(DrawLine(getPoint(templist, i), getPoint(templist, i + 0.001), 1,
                                     sf::Color::Cyan));
            }
            window.draw(DrawLine(templist.at(0), templist.at(1), 2, sf::Color::White));
            window.draw(DrawLine(templist.at(2), templist.at(3), 2, sf::Color::White));
            iterated++;
        }
    }

    FRAME+=0.01;
    if (FRAME > 1) {
        FRAME = 0;
    }
    window.display();
}

int main() {
    sf::RenderWindow window;

    try {
        // Initialize window
        window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Bezier Curve Editor");
        window.setFramerateLimit(FPS_LIMIT);
        // Prevent key repeats.
        window.setKeyRepeatEnabled(false);

        bool shouldQuit = false;
        // Main game loop
        while (window.isOpen()) {
            handleInput(window, shouldQuit);
            if (shouldQuit) {
                break;
            }
            render(window);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
