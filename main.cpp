#include <iostream>
#include <SDL2/SDL.h>
#include <limits>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>

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

    scp((g_window = SDL_CreateWindow("ddd", 0, 0, screen_width, screen_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)),
            "Could not create window");

    scp((g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)),
            "Could not create renderer");

    SDL_SetRelativeMouseMode(SDL_TRUE);
}


typedef SDL_FPoint Point2D;
typedef Point2D Vec2;

struct Vec3 {
    float x;
    float y;
    float z;

    float dot(const Vec3 &v) {
        return x * v.x + y * v.y + z * v.z;
    }
    Vec3 cross(const Vec3 &v) {
        return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
    }
    Vec3 &normalise() {
        float len = sqrtf(x * x + y * y + z * z);
        x /= len; 
        y /= len; 
        z /= len;
        return *this;
    }
};

std::ostream & operator<<(std::ostream &stream, const Vec3 &p) {
    stream << "(" << p.x << ", " << p.y << ", " << p.z << ")";
    return stream;
}
std::ostream & operator<<(std::ostream &stream, const Point2D &p) {
    stream << "(" << p.x << ", " << p.y << ")";
    return stream;
}

Point2D operator*(const Point2D &p, float a) {
    return {p.x * a, p.y * a};
}
Point2D operator+(const Point2D &p1, const Point2D &p2) {
    return {p1.x + p2.x, p1.y + p2.y};
}
Point2D operator-(const Point2D &p1, const Point2D &p2) {
    return {p1.x - p2.x, p1.y - p2.y};
}

Point2D &operator+=(Point2D &p1, const Point2D &p2) {
    p1.x += p2.x;
    p1.y += p2.y;
    return p1;
}
Point2D &operator-=(Point2D &p1, const Point2D &p2) {
        p1.x -= p2.x;
        p1.y -= p2.y;
        return p1;
}

Vec3 operator*(const Vec3 &p, float a) {
    return {p.x * a, p.y * a, p.z * a};
}
Vec3 operator+(const Vec3 &p1, const Vec3 &p2) {
    return {p1.x + p2.x, p1.y + p2.y, p1.z + p2.z};
}
Vec3 operator-(const Vec3 &p1, const Vec3 &p2) {
    return {p1.x - p2.x, p1.y - p2.y, p1.z - p2.z};
}
Vec3 operator-(const Vec3 &p) {
    return {-p.x, -p.y, -p.z};
}

Vec3 round(Vec3 p, float size = 1.0) {
    return {std::round(p.x / size) * size, std::round(p.y / size) * size, std::round(p.z / size) * size};
}

typedef Vec3 Point3D;

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

#define COLOR_RED 0xFF0333FF
#define COLOR_GREEN 0x00FF00FF
#define COLOR_BEIGE 0xF5F5DCFF

#define UNHEX(color) color >> 3 * 8, color >> 2 * 8 & 0xFF, color >> 8 & 0xFF, color & 0xFF

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

    Point3D in_front(float distance = 1) {
        return {m_pos.x + sin(-m_vertical_view_angle + (float) M_PI / 2) * cos(-m_horizontal_view_angle + (float) M_PI / 2) * distance,
        m_pos.y + cos(-m_vertical_view_angle + (float) M_PI / 2) * distance,
        m_pos.z + sin(-m_horizontal_view_angle + (float) M_PI / 2) * sin(-m_vertical_view_angle + (float) M_PI / 2) * distance};
    }

    Point3D m_pos;
    float m_vertical_view_angle;
    float m_horizontal_view_angle;
};

/******************** Projection functions **********************************/

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
    if (p3.z < 0.1) p3.z = 0.1; // the coordinate is too big (overflows) for small z, so we clamp it
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

inline Point2D project_with_camera(Point3D p, Player& player, Point3D &rotated) {
    rotated = rotate_y(-player.m_horizontal_view_angle, p - player.m_pos);
    rotated = rotate_x(player.m_vertical_view_angle, rotated);
    return project(rotated + player.m_pos, player);
}

inline Point3D get_camera_coords(Point3D p, Player& player) {
    Point3D rotated = rotate_y(-player.m_horizontal_view_angle, p - player.m_pos);
    rotated = rotate_x(player.m_vertical_view_angle, rotated);
    return rotated + player.m_pos;
}

inline Point2D place_projected_point(Point2D point) {
    //TODO: properly cut down lines that exceed the screen bounds (clipping)
    return { std::clamp((point.x * screen_width) + screen_width  * 0.5f, -1000.0f, (float) screen_width  + 1000.0f),
             std::clamp((point.y * screen_width) + screen_height * 0.5f, -1000.0f, (float) screen_height + 1000.0f) };
}

inline Point2D get_onscreen_point(Point3D p, Player &player) {
    return place_projected_point(project_with_camera(p, player));
}

/****************************************************************************/

class Drawable {
public:
    void draw(SDL_Renderer *renderer, Player &player) {
        if (active)
            draw_impl(renderer, player);
    }
    bool switch_activation() {
        return active = !active;
    }
protected:
    bool active = true;
private:
    virtual void draw_impl(SDL_Renderer *renderer, Player &player) = 0;
};

struct Triangle {
    Point3D vertices[3];

    Triangle to_camera_coords(Player &player) {
        return {get_camera_coords(vertices[0], player), get_camera_coords(vertices[1], player), get_camera_coords(vertices[2], player)};
    }
    Point3D get_normal() {
        return (vertices[1] - vertices[0]).cross(vertices[2] - vertices[0]).normalise();
    }
};

struct Cube : Drawable {
    Cube(Point3D pos, float scale, uint32_t color = COLOR_RED) : m_pos(pos), m_scale(scale), m_color(color) {
        /*
         * pos   - position of the center of the cube
         * scale - the size of the cube
         *
         */
    }

    void draw_impl(SDL_Renderer *renderer, Player &player) {
        // draw order is: 0-1-2-3-4-5-6-7-0-3, 1-6, 2-5, 4-7
        
        SDL_SetRenderDrawColor(renderer, UNHEX(m_color));

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
    uint32_t m_color;
};

struct Mesh : Drawable {

    Mesh(std::string filepath) : m_polygons(nullptr), m_count(0) {
        /* create a mesh from an obj file */
        std::ifstream f = std::ifstream(filepath);
        if (!f.is_open()) {
            std::cerr << "Error: failed to open " << filepath << '\n';
            return;
        }
        std::vector<Vec3> verts;
        std::vector<Vec3> normals;
        std::vector<Triangle> tris;
        int v_count = 0, n_count = 0;
        char c;
        int line_number = 0;
        while (!f.eof()) {
            std::string line;
            std::getline(f, line);
            ++line_number;
            if (line.size() == 0 || line[0] == ' ') continue;
            std::istringstream strstream(line);
            strstream >> c;
            switch (c) {
                case '#':
                case 'g':
                    // do nothing
                    break;
                case 'v':
                    if (strstream.peek() == 'n') {
                        continue;
                    }
                    else {
                        //vertex
                        verts.push_back(parse_v3(strstream));
                    }
                    break;
                case 'f':
                    int v[3], n[3];
                    for (int i = 0; i < 3; ++i) {
                        strstream >> v[i];
                        // ignore "//"
                        if (strstream.peek() == '/') {
                            strstream.ignore(2);
                            strstream >> n[i];
                        }
                    }
                    tris.push_back({{verts[v[0] - 1], verts[v[1] - 1], verts[v[2] - 1]}});
                    break;
                default:
                    std::cerr << "Unknown symbol at line " << line_number << ": '" << c << '\'' << '\n';
            }
        }
        m_count = tris.size();
        m_polygons = new Triangle[m_count];
        std::copy(tris.begin(), tris.end(), m_polygons);
    }

    void draw_impl(SDL_Renderer *renderer, Player &player) {
        Triangle tris[m_count];
        for (int i = 0; i < m_count; ++i) {
            tris[i] = m_polygons[i].to_camera_coords(player);
        }
        //sort by z
        //TODO: depth buffer
        std::sort(tris, tris + m_count, [](Triangle &t1, Triangle &t2) {
                float z1 = t1.vertices[0].z + t1.vertices[1].z + t1.vertices[2].z;
                float z2 = t2.vertices[0].z + t2.vertices[1].z + t2.vertices[2].z;
                return z1 > z2;
        });
        Point2D pts[4];

        Vec3 light_direction = { 0.0f, 0.0f, 1.0f };
        light_direction.normalise();
        Point3D normal;

        for (int i = 0; i < m_count; ++i) {
            if ((normal = tris[i].get_normal()).dot(tris[i].vertices[0] - player.m_pos) >= 0) continue;
            for (int j = 0; j < 3; ++j) {
                pts[j] = place_projected_point(project(tris[i].vertices[j], player));
            }
            
            uint8_t col = (-light_direction.dot(normal)) * 255;

            SDL_Vertex verts[3] =
            {
                { pts[0], SDL_Color{ col, col, col, 0xFF }, SDL_FPoint{ 0 }, },
                { pts[1], SDL_Color{ col, col, col, 0xFF }, SDL_FPoint{ 0 }, },
                { pts[2], SDL_Color{ col, col, col, 0xFF }, SDL_FPoint{ 0 }, },
            };
            SDL_RenderGeometry(renderer, nullptr, verts, 3, nullptr, 0);
        }
    }

    void translate(float x, float y, float z) {
        for (int i = 0; i < m_count; ++i) {
            for (int j = 0; j < 3; ++j) {
                m_polygons[i].vertices[j].x += x;
                m_polygons[i].vertices[j].y += y;
                m_polygons[i].vertices[j].z += z;
            }
        }
    }

    void rotate(float angle) {
        for (int i = 0; i < m_count; ++i) {
            for (int j = 0; j < 3; ++j) {
                m_polygons[i].vertices[j] = rotate_y(angle, m_polygons[i].vertices[j]);
            }
        }
    }

    void rotate_around_point(float angle, Point3D origin) {
        for (int i = 0; i < m_count; ++i) {
            for (int j = 0; j < 3; ++j) {
                m_polygons[i].vertices[j] = rotate_y_around_point(angle, m_polygons[i].vertices[j], origin);
            }
        }
    }

private:
    Triangle *m_polygons;
    int m_count;

    Vec3 parse_v3(std::istringstream &f) {
        Vec3 v;
        f >> v.x >> v.y >> v.z;
        return v;
    }
};

struct Axes : Drawable {

    Axes(float length) : m_length(length) { }

    void draw_impl(SDL_Renderer *renderer, Player &player) {
        
        // draw in front of the player
        Point3D pos = player.in_front();

        Point2D offset = { 0.35f * screen_width, -0.35f * screen_height };
        Point2D origin = { 0.5f * screen_width,  0.5f * screen_height };
        origin += offset;

        Point2D x = get_onscreen_point(pos + (Point3D) {m_length, 0, 0}, player) + offset;
        Point2D y = get_onscreen_point(pos + (Point3D) {0, m_length, 0}, player) + offset;
        Point2D z = get_onscreen_point(pos + (Point3D) {0, 0, m_length}, player) + offset;


        SDL_SetRenderDrawColor(g_renderer, 0x31, 0x31, 0x31, 0xFF);
#define RECT_SIZE (screen_height + screen_width) / 12
        SDL_Rect axis_rect = { (int) (0.85f * screen_width) - RECT_SIZE / 2, (int) (0.15f * screen_height) - RECT_SIZE / 2, RECT_SIZE, RECT_SIZE};
        SDL_RenderFillRect(g_renderer, &axis_rect);
        SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);

        // draw x axis
        SDL_RenderDrawLineF(renderer, origin.x, origin.y, x.x, x.y);
        SDL_RenderDrawLineF(renderer, x.x - 10, x.y - 20, x.x + 10, x.y);
        SDL_RenderDrawLineF(renderer, x.x - 10, x.y,      x.x + 10, x.y - 20);

        // draw y axis
        SDL_RenderDrawLineF(renderer, origin.x, origin.y, y.x, y.y);
        SDL_RenderDrawLineF(renderer, y.x + 15, y.y,      y.x + 15, y.y - 10);
        SDL_RenderDrawLineF(renderer, y.x + 15, y.y - 10, y.x + 5,  y.y - 20);
        SDL_RenderDrawLineF(renderer, y.x + 15, y.y - 10, y.x + 25, y.y - 20);

        //draw z axis
        SDL_RenderDrawLineF(renderer, origin.x, origin.y, z.x, z.y);
        SDL_RenderDrawLineF(renderer, z.x - 10, z.y - 25, z.x + 10, z.y - 25);
        SDL_RenderDrawLineF(renderer, z.x + 10, z.y - 25, z.x - 10, z.y - 5);
        SDL_RenderDrawLineF(renderer, z.x - 10, z.y - 5,  z.x + 10, z.y - 5);
        

    }

private:
    float m_length;
};

//TODO: rewrite everything in terms of matrices to be able to transform meshes in any way
struct Space_ship : Drawable {

    void move_forward(float d) {
        m_pos.z += std::cos(m_angle) * d;
        m_pos.x += std::sin(m_angle) * d;
    }

    void rotate(float angle) {
        m_angle += angle;
    }

    void draw_impl(SDL_Renderer *renderer, Player &player) {
        m_mesh.draw_impl(renderer, player);
    }

private:
    Point3D m_pos;
    Mesh m_mesh;
    float m_angle;
};

int main () {

    bool quit;
    init();
    SDL_Event event;

    Player player(0, -.7, 0);
    int input_timer_start = SDL_GetTicks();

    std::vector<Drawable *> drawing_list;

    Axes axes(0.1);
    drawing_list.push_back(&axes);
    
    Mesh mesh("ship.obj");
    float ship_angle = 0;
    mesh.translate(0, 0, -25.0);
    mesh.rotate_around_point(-M_PI / 2.0f, {0.0f, 0.0f, -25.0f});
    drawing_list.push_back(&mesh);

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
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        drawing_list.push_back(new Cube(round(player.in_front(1.2f), 0.5f), 0.5f, COLOR_RED));
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        drawing_list.push_back(new Cube(round(player.in_front(1.2f), 0.5f), 0.5f, COLOR_GREEN));
                    }
                case SDL_KEYDOWN:
                    if (event.key.keysym.scancode == SDL_SCANCODE_F1) {
                        axes.switch_activation();
                    }
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
            if (key_state[SDL_SCANCODE_SPACE]) {
                player.move_y(-0.1);
            }
        }

        SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(g_renderer);

        /***************** Drawing *******************************/

        mesh.rotate(ship_angle+=0.000001);

        for (auto i: drawing_list) {
            i->draw(g_renderer, player);
        }

        //cross
        SDL_SetRenderDrawColor(g_renderer, UNHEX(COLOR_BEIGE));
        SDL_RenderDrawLine(g_renderer, screen_width / 2, screen_height / 2 - 10, screen_width / 2, screen_height / 2 + 10);
        SDL_RenderDrawLine(g_renderer, screen_width / 2 - 10, screen_height / 2, screen_width / 2 + 10, screen_height / 2);

        /*********************************************************/

        SDL_RenderPresent(g_renderer);
    }

    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return 0;
}
