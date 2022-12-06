#include <iostream>
#include <SDL2/SDL.h>
#include <limits>
#include <vector>
#include <algorithm>

#define scp(pointer, message) {                                               \
    if (pointer == NULL) {                                                    \
        SDL_Log("Error: %s! SDL_Error: %s", message, SDL_GetError()); \
        exit(1);                                                              \
    }                                                                         \
}

#define scc(code, message) {                                                  \
    if (code < 0) {                                                           \
        SDL_Log("Error: %s! SDL_Error: %s", message, SDL_GetError()); \
        exit(1);                                                              \
    }                                                                         \
}

SDL_Window * g_window;
SDL_Renderer *g_renderer;

int screen_width;
int screen_height;

void init() {

	scc(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER), "Could not initialize SDL");
    if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        SDL_Log("Warning: Linear texture filtering not enabled!");
    }

    scp((g_window = SDL_CreateWindow("Can'ts", 0, 0, screen_width, screen_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)),
            "Could not create window");

    scp((g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)),
            "Could not create renderer");

    SDL_SetRelativeMouseMode(SDL_TRUE);
}


typedef SDL_FPoint Point2D;

struct Point3D {
    float x;
    float y;
    float z;
};

std::ostream & operator<<(std::ostream &stream, const Point3D &p) {
    stream << "(" << p.x << ", " << p.y << ", " << p.z << ")";
    return stream;
}
std::ostream & operator<<(std::ostream &stream, const Point2D &p) {
    stream << "(" << p.x << ", " << p.y << ")";
    return stream;
}

Point3D operator*(const Point3D &p, float a) {
    return {p.x * a, p.y * a, p.z * a};
}
Point3D operator+(const Point3D &p1, const Point3D &p2) {
    return {p1.x + p2.x, p1.y + p2.y, p1.z + p2.z};
}
Point3D operator-(const Point3D &p1, const Point3D &p2) {
    return {p1.x - p2.x, p1.y - p2.y, p1.z - p2.z};
}

typedef Point3D Face[4];

const Face cube_faces[] = {
    {
        { -0.5,  0.5,  -0.5 },
        {  0.5,  0.5,  -0.5 },
        {  0.5,  0.5,  0.5 },
        {  -0.5,  0.5,  0.5 },
    },
    {
        {  -0.5,  -0.5,  -0.5 },
        {  0.5,  -0.5,  -0.5 },
        {  0.5,  -0.5,  0.5 },
        {  -0.5,  -0.5,  0.5 },
  },
    {
    {  -0.5,  -0.5,  0.5 },
    {  0.5,  -0.5,  0.5 },
    {  0.5,  0.5,  0.5 },
    {  -0.5,  0.5,  0.5 },
  },
  {
    {  -0.5,  -0.5,  -0.5 },
    {  0.5,  -0.5,  -0.5 },
    {  0.5,  0.5,  -0.5 },
    {  -0.5,  0.5,  -0.5 },
  },
};

const Point3D cube_points[] {
    { -0.5,  0.5,  -0.5 },
    {  0.5,  0.5,  -0.5 },
    {  0.5,  0.5,   0.5 },
    { -0.5,  0.5,   0.5 },
    { -0.5, -0.5,   0.5 },
    {  0.5, -0.5,   0.5 },
    {  0.5, -0.5,  -0.5 },
    { -0.5, -0.5,  -0.5 },
};


#define FOV_IN_DEGREES 120.0
const double FOV = FOV_IN_DEGREES * M_PI / 180;
const float tana2 = tan(FOV / 2);

struct Player {

    Player(float x, float y, float z) 
    : m_pos({x, y, z}), m_horizontal_view_angle(0.0), m_vertical_view_angle(0.0) {}

    void move_forward(float d) {
        m_pos.z += std::cos(m_horizontal_view_angle) * d;
        m_pos.x += std::sin(m_horizontal_view_angle) * d;
    }

    void move_backward(float d) {
        move_forward(-d);
    }

    void move_left(float d) {
        m_pos.z += std::sin(m_horizontal_view_angle) * d;
        m_pos.x -= std::cos(m_horizontal_view_angle) * d;
    }
    void move_right(float d) {
        move_left(-d);
    }

    void move_x(float dx) {
        m_pos.x += dx;
    }

    void move_y(float dy) {
        m_pos.y += dy;
    }

    void move_z(float dz) {
        m_pos.z += dz;
    }

    Point3D m_pos;
    float m_vertical_view_angle;
    float m_horizontal_view_angle;
};

/******************** Projection functions **********************************/

inline Point2D project(Point3D p3) {
    if (p3.z < 1) p3.z = 1;
    return { p3.x / (p3.z * tana2), p3.y / (p3.z * tana2)};
}

inline Point3D rotate_y(float angle, Point3D p) {
    return { p.x * cos(angle) + p.z * sin(angle),
             p.y,
             -sin(angle) * p.x + cos(angle) * p.z };
}
inline Point3D rotate_x(float angle, Point3D p) {
    return { p.x, 
             p.y * cos(angle) - p.z * sin(angle),
             p.y * sin(angle) + p.z * cos(angle)};
}

inline Point3D rotate_y_around_point(float angle, Point3D p, Point3D origin) {
    Point3D p1 = p - origin;
    p1 = rotate_y(angle, p1);
    return p1 + origin;
}

inline Point2D project(Point3D p3, Player &player) {
    p3 = p3 - player.m_pos;
    if (p3.z < 0.01) p3.z = 0.01; // the coordinate is too big (overflows) for small z, so we clamp it
    return { p3.x / (p3.z * tana2), p3.y / (p3.z * tana2)};
}

inline Point2D project_with_camera_horizontal(Point3D p, Player& player) {
    return project(rotate_y_around_point(-player.m_horizontal_view_angle, p, player.m_pos), player);
}

inline Point2D project_with_camera(Point3D p, Player& player) {
    Point3D p1 = rotate_y(-player.m_horizontal_view_angle, p - player.m_pos);
            p1 = rotate_x(player.m_vertical_view_angle,   p1);
    return project(p1 + player.m_pos, player);
}

/****************************************************************************/

inline Point2D place_projected_point(Point2D point) {
    //TODO: properly cut down lines that exceed the screen bounds
    return { std::clamp((point.x * screen_width) + screen_width  * 0.5f, -1000.0f, (float) screen_width  + 1000.0f),
             std::clamp((point.y * screen_width) + screen_height * 0.5f, -1000.0f, (float) screen_height + 1000.0f) };
}

inline Point2D get_onscreen_point(Point3D p, Player &player) {
    return place_projected_point(project_with_camera(p, player));
}

class Drawable {
public:
    virtual void draw(SDL_Renderer *renderer, Player &player) = 0;
};

//TODO: smart polygon drawing (optimize out drawing the same edge)
struct Mesh {

    Mesh(Point3D **vertices, int v_count, float scale, Point3D pos) : m_vertices(vertices), m_count(v_count),
                                                                    m_scale(scale), m_pos(pos) {}

    virtual void draw(SDL_Renderer *renderer, Player &player) {
        Point2D pts[m_count];
        for (int i = 0; i < m_count; ++i) {
            pts[i] = get_onscreen_point(*m_vertices[i] * m_scale + m_pos, player);
        }
        SDL_RenderDrawLinesF(renderer, pts, m_count);
    }

private:
    Point3D **m_vertices;
    int m_count;
    int m_scale;
    Point3D m_pos;
};

struct Cube : Drawable {
    Cube(Point3D pos, float scale) : m_pos(pos), m_scale(scale) {
        /*
         * pos   - position of the center of the cube
         * scale - the size of the cube
         *
         */
    }

    void draw(SDL_Renderer *renderer, Player &player) {
        // draw order is: 0-1-2-3-4-5-6-7-0-3, 1-6, 2-5, 4-7

        Point2D pts[10];
        for (int i = 0; i < 8; ++i) {
            pts[i] = get_onscreen_point(cube_points[i] * m_scale + m_pos, player);
        }
        pts[8] = pts[0];
        pts[9] = pts[3];
        SDL_RenderDrawLinesF(renderer, pts, 10);
        SDL_RenderDrawLineF(renderer, pts[1].x, pts[1].y, pts[6].x, pts[6].y);
        SDL_RenderDrawLineF(renderer, pts[2].x, pts[2].y, pts[5].x, pts[5].y);
        SDL_RenderDrawLineF(renderer, pts[4].x, pts[4].y, pts[7].x, pts[7].y);
    }

    void move(float x, float y = 0, float z = 0) {
        m_pos.x += x;
        m_pos.y += y;
        m_pos.z += z;
    }

    Point3D getpos() {
        return m_pos;
    }

    float get_top() {
        return m_pos.y - m_scale * 0.5;
    }

private:
    float m_scale;
    Point3D m_pos;

};


struct Axes : Drawable {

    Axes(Point3D pos, float length) : m_pos(pos), m_length(length) {}

    void draw(SDL_Renderer *renderer, Player &player) {
        
        Point2D origin = get_onscreen_point(m_pos, player);

        Point2D x = get_onscreen_point(m_pos + (Point3D) {1 * m_length, 0, 0}, player);
        Point2D y = get_onscreen_point(m_pos + (Point3D) {0, 1 * m_length, 0}, player);
        Point2D z = get_onscreen_point(m_pos + (Point3D) {0, 0, 1 * m_length}, player);

        // draw x axis
        SDL_RenderDrawLineF(renderer, origin.x, origin.y, x.x, x.y);
        SDL_RenderDrawLineF(renderer, x.x - 10, x.y - 20, x.x + 10, x.y);
        SDL_RenderDrawLineF(renderer, x.x - 10, x.y, x.x + 10, x.y - 20);

        // draw y axis
        SDL_RenderDrawLineF(renderer, origin.x, origin.y, y.x, y.y);
        SDL_RenderDrawLineF(renderer, y.x + 15, y.y,      y.x + 15, y.y - 10);
        SDL_RenderDrawLineF(renderer, y.x + 15, y.y - 10, y.x + 5,  y.y - 20);
        SDL_RenderDrawLineF(renderer, y.x + 15, y.y - 10, y.x + 25, y.y - 20);

        SDL_RenderDrawLineF(renderer, origin.x, origin.y, z.x, z.y);
        SDL_RenderDrawLineF(renderer, z.x - 10, z.y - 5, z.x + 10, z.y - 5);
        SDL_RenderDrawLineF(renderer, z.x + 10, z.y - 5, z.x - 10, z.y + 20);
        SDL_RenderDrawLineF(renderer, z.x - 10, z.y + 20, z.x + 10, z.y + 20);

    }

private:
    Point3D m_pos;
    float m_length;
};


int main () {
    bool quit;
    init();
    SDL_Event event;

    Player player(0, -1.1, 0);
    int input_timer_start = SDL_GetTicks();

    std::vector<Drawable *> drawing_list;

#if 0
    Cube *cubes[16];

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            cubes[i * 4 + j] = new Cube({(float) i - 2, 1.1, 3 + (float) j}, 1);
        }
    }
#else
    Cube cube({0, 1.1, 3}, 1);
    drawing_list.push_back(&cube);

#endif

    Axes axes({1, 0.5, 3}, 1);
    drawing_list.push_back(&axes);


    while (!quit) {
        while(SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        screen_width = event.window.data1;
                        screen_height = event.window.data2;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    player.m_vertical_view_angle   = std::clamp(player.m_vertical_view_angle + (double) event.motion.yrel / 400, -M_PI / 2, M_PI / 2); // can only look 90 degrees up
                    player.m_horizontal_view_angle = player.m_horizontal_view_angle + (double) event.motion.xrel / 400;
                    break;
            }
        }

        if (SDL_GetTicks() - input_timer_start > 1000 / 30) {
            input_timer_start = SDL_GetTicks();
            const Uint8* key_state = SDL_GetKeyboardState(NULL);
            const SDL_Keymod mod_state = SDL_GetModState();
            if (key_state[SDL_SCANCODE_D]) {
                player.move_right(0.1);
            }
            if (key_state[SDL_SCANCODE_A]) {
                player.move_left(0.1);
            }
            if (key_state[SDL_SCANCODE_W]) {
                player.move_forward(0.1);
            }
            if (key_state[SDL_SCANCODE_S]) {
                player.move_backward(0.1);
            }
            if (mod_state & KMOD_CTRL) {
                player.move_y(0.1);
            }
            if (mod_state & KMOD_LSHIFT) {
                player.move_y(-0.1);
            }
        }

        SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(g_renderer);

        /***************** Drawing *******************************/

        SDL_SetRenderDrawColor(g_renderer, 0xFF, 0x33, 0x33, 0xFF);

        for (auto i: drawing_list) {
            i->draw(g_renderer, player);
        }

        /*********************************************************/

        SDL_RenderPresent(g_renderer);
    }

    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return 0;
}
