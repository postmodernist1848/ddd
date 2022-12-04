#include <iostream>
#include <SDL2/SDL.h>
#include <cmath>
#include <limits>

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

typedef Point3D Face[4];

const Face cube_faces[] = {
  // Bottom
    {
        { -0.5,  0.5,  -0.5 },
        {  0.5,  0.5,  -0.5 },
        {  0.5,  0.5,  0.5 },
        {  -0.5,  0.5,  0.5 },
    },
  // Top
    {
    {  -0.5,  -0.5,  -0.5 },
    {  0.5,  -0.5,  -0.5 },
    {  0.5,  -0.5,  0.5 },
    {  -0.5,  -0.5,  0.5 },
  },
  // Front
    {
    {  -0.5,  -0.5,  0.5 },
    {  0.5,  -0.5,  0.5 },
    {  0.5,  0.5,  0.5 },
    {  -0.5,  0.5,  0.5 },
  },
  // Back
  {
    {  -0.5,  -0.5,  -0.5 },
    {  0.5,  -0.5,  -0.5 },
    {  0.5,  0.5,  -0.5 },
    {  -0.5,  0.5,  -0.5 },
  },
};

#define FOV_IN_DEGREES 45.0
const double FOV = FOV_IN_DEGREES * M_PI / 180;
const float tana2 = tan(FOV / 2);

void draw_line(Point2D p1, Point2D p2) {
    SDL_RenderDrawLineF(g_renderer, p1.x + (float) screen_width / 2, p1.y + (float) screen_height / 2, p2.x + (float) screen_width / 2, p2.y + (float) screen_height / 2);
}

Point2D project(Point3D p3) {
    return { p3.x / (p3.z * tana2), p3.y / (p3.z * tana2)};
}

struct Player {

    Player(float x, float y, float z) : m_pos({x, y, z}) {}

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
};

struct Cube {
    Cube(Point3D pos, float scale) : m_pos(pos), m_scale(scale) {
        /*
         * pos   - position of the center of the cube
         * scale - the size of the cube
         *
         */
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                m_faces[i][j].x = cube_faces[i][j].x * scale + pos.x;
                m_faces[i][j].y = cube_faces[i][j].y * scale + pos.y;
                m_faces[i][j].z = cube_faces[i][j].z + pos.z;
            }
        }
    }

    void draw(SDL_Renderer *renderer, Player &player) {
        if (m_pos.z > 1) {
            for (int i = 0; i < 4; ++i) {
                Point2D pts[5];
                for (int j = 0; j < 4; ++j) {
                    pts[j] = project(m_faces[i][j]);
                    pts[j].x = (pts[j].x * screen_width) + screen_width * 0.5;
                    pts[j].y = (pts[j].y * screen_width) + screen_height * 0.5;
                }
                pts[4] = pts[0];
                std::cout << pts[0] << "\n";
                std::cout << getpos() << "\n";
                //TODO: figure out why this leaks tons of memory when used with z == 0.5
                SDL_RenderDrawLinesF(renderer, pts, 5);
            }
        }
    }

    void move(float x, float y = 0, float z = 0) {
        m_pos.x += x;
        m_pos.y += y;
        m_pos.z += z;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                m_faces[i][j].x += x;
                m_faces[i][j].y += y;
                m_faces[i][j].z += z;
            }
        }
    }

    Point3D getpos() {
        return m_pos;
    }

private:
    Face m_faces[4];
    float m_scale;
    Point3D m_pos;
};


int main () {
    bool quit;
    init();
    SDL_Event event;

    Player player(0, 0, 0);

    Cube a = Cube({-2, 2, 10}, 2);
    Cube b = Cube({0, 2, 12}, 2);
    Cube c = Cube({0, 2, 10}, 2);

    std::cout << "The near plane size is: " << 1.0 / tana2 << std::endl;

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
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    const Uint8* state = SDL_GetKeyboardState(NULL);
                    if (state[SDL_SCANCODE_D]) {
                        player.move_x(1);
                    }
                    if (state[SDL_SCANCODE_A]) {
                        player.move_x(-1);
                    }
                    if (state[SDL_SCANCODE_W]) {
                        player.move_z(1);
                    }
                    if (state[SDL_SCANCODE_S]) {
                        player.move_z(-1);
                    }
                    if (state[SDL_SCANCODE_UP]) {
                        player.move_y(1);
                    }
                    if (state[SDL_SCANCODE_DOWN]) {
                        player.move_y(-1);
                    }
                    break;
            }
        }
        SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(g_renderer);

        /***************** Drawing *******************************/

        SDL_SetRenderDrawColor(g_renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        a.draw(g_renderer, player);
        b.draw(g_renderer, player);
        c.draw(g_renderer, player);

        /*********************************************************/

        SDL_RenderPresent(g_renderer);
    }

    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return 0;
}
