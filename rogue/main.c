#include <curses.h> // pdcurses ���̺귯��
#include <stdlib.h>
#include <time.h>

#define VISIBLE_COLOR 1 // ���� ���� ����
#define SEEN_COLOR 2    // �ν� ���� ����

typedef struct
{
    int y;  // y ��ǥ
    int x;  // x ��ǥ
} Position;	// ��ǥ ����ü

typedef struct
{
    char ch;    // Ÿ�� ����
    int color;  // Ÿ�� ��
    bool walkable;  // Ÿ�� ���� ���� ����
    bool transparent;   // Ÿ�� �þ� ��� ����
    bool visible;   // Ÿ�� ���� ���� ����
    bool seen;  // Ÿ�� �ν� ���� ����
} Tile; // Ÿ�� ����ü

typedef struct
{
    int height; // �� ����
    int width;  // �� �ʺ�
    Position pos;   // �� ��ǥ (���� ���)
    Position center;    // �� ��ǥ (�߽�)
} Room; // �� ����ü

typedef struct
{
    Position pos;	// ��ü ��ǥ
    char ch;	// ��ü ����
    int color;  // ��ü ��
} Entity;	// ��ü ����ü

// draw �Լ�
void drawMap(void); // �� ��� �Լ�
void drawEntity(Entity* entity);    // ��ü ��� �Լ�
void drawEverything(void);  // ȭ�� ��� �Լ�

// engine �Լ�
void cursesSetup(void); // �ʱ�ȭ �Լ�
void gameLoop(void);    // �� ���� �Լ�
void closeGame(void);   // ���� ���� �Լ�

// map �Լ�
Tile** createMapTiles(void);    // �� ���� �Լ�
Position setupMap(void);    // �� ���� ���� �Լ�
void freeMap(void); // �� ���� �޸� ���� �Լ�

// player �Լ�
Entity* createPlayer(Position start_pos);   // �÷��̾� ���� �Լ�
void handleInput(int input);    // ���� ���� �Լ�
void movePlayer(Position newPos);   // �÷��̾� �̵� �Լ�

// room �Լ�
Room createRoom(int y, int x, int height, int width);   // �� ���� �Լ�
void addRoomToMap(Room room);   // �ʿ� �� �߰� �Լ�
void connectRoomCenters(Position centerOne, Position centerTwo);    // �� �� ���� �Լ�

// fov �Լ�
void makeFOV(Entity* player);   // ���� �þ� ���� �Լ�
void clearFOV(Entity * player);
int getDistance(Position origin, Position target);  // �÷��̾�� ��� �� �Ÿ� ��� �Լ�
bool isInMap(int y, int x); // ��ǥ �� ���� ���� �Ǻ� �Լ�
bool lineOfSight(Position origin, Position target); // ��ǥ ���� �þ� ���� �Ǻ� �Լ� (Bresenham's Line Drawing Algorithm)
int getSign(int a); // ���� ���� �Ǻ� �Լ�

// �������� & ��������(const)
const int MAP_HEIGHT = 25;  // �� ����
const int MAP_WIDTH = 100;  // �� �ʺ�

// ��������
Entity* player; // �÷��̾� (��ü ����ü ������)
Tile** map; // �� (Ÿ�� ����ü ���� ������ = Ÿ�� ����ü 2���� �迭)

// draw
void drawMap(void)  // �� ��� �Լ�
{
    for (int y = 0; y < MAP_HEIGHT; y++)    // �� ���̸�ŭ �ݺ�
    {
        for (int x = 0; x < MAP_WIDTH; x++) // �� �ʺ�ŭ �ݺ�
        {
            if (map[y][x].visible)  // �ش� ��ǥ�� ����(�þ� ���� ��) Ÿ���� ���
            {
                // mvaddch(y��ǥ, x��ǥ, ���� | ����); (curses.h)
                mvaddch(y, x, map[y][x].ch | map[y][x].color);  // �ش� Ÿ�Ͽ� ����� ����(����� ������) ���
            }
            else if (map[y][x].seen)    // �ش� ��ǥ�� ���� Ÿ���� �ƴϰ� �ν�(�þ� ���� ���� �� ���̶� ����) Ÿ���� ���
            {
                mvaddch(y, x, map[y][x].ch | COLOR_PAIR(SEEN_COLOR));   // �ش� Ÿ�Ͽ� ����� ����(�ν� ������) ���
            }
            else    // �ش� ��ǥ�� �� ���� �þ� ������ ���� �� ���� Ÿ���� ���
            {
                mvaddch(y, x, ' '); // �������� ���
            }
        }
    }
}

void drawEntity(Entity* entity) // ��ü ��� �Լ�
{
    mvaddch(entity->pos.y, entity->pos.x, entity->ch | entity->color);  // ��ü ��ǥ�� ��ü ����(��ü ��������) ���
}

void drawEverything(void)   // ȭ�� ��� �Լ�
{
    clear();    // �ܼ� â �ʱ�ȭ
    drawMap();  // �� ���
    drawEntity(player); // ��ü ���
}

// engine
void cursesSetup(void)  // �ʱ�ȭ �Լ�
{
    initscr();  // curses ��� ����
    noecho();   // ����ڷκ��� �Է¹��� ���� ��� X
    curs_set(0);    // Ŀ�� ������ �ʰ�

    if (has_colors())   // �ܼ��� ���� ��� ������ ���
    {
        start_color();  // color ��� ����

        // init_pair(�ȷ�Ʈ �̸�, ���ڻ�, ����); (curses.h)
        init_pair(VISIBLE_COLOR, COLOR_WHITE, COLOR_BLACK); // ���� ���� �ȷ�Ʈ ����
        init_pair(SEEN_COLOR, COLOR_BLUE, COLOR_BLACK); // �ν� ���� �ȷ�Ʈ ����
    }
}

void gameLoop(void) // �� ���� �Լ�
{
    int ch; // ����� �Է� ���� ����

    makeFOV(player);    // �ʱ� �þ� ����
    drawEverything();   // ȭ�� ���

    while (ch = getch())    // ���ڸ� �Է¹޾��� ���
    {
        if (ch == 'q')  // q�� �Է¹޾��� ���
        {
            break;  // ���� ����
        }

        handleInput(ch);    // ���� ���� �Լ��� �Է� ���� ����
        drawEverything();   // ȭ�� ���
    }
}

void closeGame(void)    // ���� ���� �Լ�
{
    endwin();   // curses ��� ����
    free(player);   // �÷��̾� ����(��ü ����ü ������) �޸� ����
}

// map
Tile** createMapTiles(void) // �� ���� �Լ�
{
    // Ÿ�� ����ü ������(�迭)�� ũ�� * �� ���̸�ŭ�� �޸� ���� �Ҵ�
    Tile** tiles = calloc(MAP_HEIGHT, sizeof(Tile*));   // Ÿ�� ���� ������(2���� �迭) = �� ����

    for (int y = 0; y < MAP_HEIGHT; y++)    // �� ���̸�ŭ �ݺ�
    {
        // Ÿ�� ����ü�� ũ�� * �� �ʺ�ŭ�� �޸� ���� �Ҵ�
        tiles[y] = calloc(MAP_WIDTH, sizeof(Tile)); // �� Ÿ�� �迭 (�� �� ���� �ǹ�)
        for (int x = 0; x < MAP_WIDTH; x++) // �� �ʺ�ŭ �ݺ�
        {
            tiles[y][x].ch = '#';   // ��� Ÿ�� ���ڸ� ���� ��Ÿ���� #���� ����
            tiles[y][x].color = COLOR_PAIR(VISIBLE_COLOR);  // ���� ���� ����
            tiles[y][x].walkable = FALSE;   // ���� �Ұ� ����
            tiles[y][x].transparent = FALSE;    // ������ ����
            tiles[y][x].visible = FALSE;    // �Ұ��� ����
            tiles[y][x].seen = FALSE;   // ���ν� ����
        }
    }

    return tiles;   // �� ����(Ÿ�� ����ü ���� ������) ��ȯ
}

Position setupMap(void) // �� ���� ���� �Լ�
{
    int y, x, height, width, n_rooms;   // �� y��ǥ, �� x��ǥ, �� ����, �� �ʺ�, �� ����
    n_rooms = (rand() % 11) + 5;    // �� ���� 5~15�� �� �������� ����
    // �� ����ü�� ũ�� * �� ������ŭ�� �޸� ���� �Ҵ�
    Room* rooms = calloc(n_rooms, sizeof(Room));    // �� ������(�迭) (��� �� ����ü�� ���� ����)
    Position start_pos; // �÷��̾� ���� ��ǥ ����

    for (int i = 0; i < n_rooms; i++)   // �� ������ŭ �ݺ�
    {
        y = (rand() % (MAP_HEIGHT - 10)) + 1;   // �� y��ǥ 1~15 �� �������� ����
        x = (rand() % (MAP_WIDTH - 20)) + 1;    // �� x��ǥ 1~80 �� �������� ����
        height = (rand() % 7) + 3;  // �� ���� 3~9 �� �������� ����
        width = (rand() % 15) + 5;  // �� �ʺ� 5~19 �� �������� ����
        rooms[i] = createRoom(y, x, height, width); // �� ���� �Լ����� ��ȯ���� ����ü�� �� ������ ����
        addRoomToMap(rooms[i]); // �ʿ� �� �߰�

        if (i > 0)  // i�� 0 �ʰ��� ��� = ���� 2�� �̻� �����Ǿ��� ���
        {
            connectRoomCenters(rooms[i - 1].center, rooms[i].center);   // ������ ��� ���� ������ �� ����
        }
    }

    // �÷��̾� ���� ��ǥ ����
    start_pos.y = rooms[0].center.y;    // �÷��̾� ���� y��ǥ = ù ��° ������ ���� �߽� y��ǥ
    start_pos.x = rooms[0].center.x;    // �÷��̾� ���� x��ǥ = ù ��° ������ ���� �߽� x��ǥ

    free(rooms);    // �� ���� �޸� ����

    return start_pos;   // �÷��̾� ���� ��ǥ(��ǥ ����ü) ��ȯ
}

void freeMap(void)  // �� ���� �޸� ���� �Լ�
{
    for (int y = 0; y < MAP_HEIGHT; y++)    // �� ���̸�ŭ �ݺ�
    {
        free(map[y]);   // �� �� ��(Ÿ�� �迭)�� �Ҵ�� �޸� ����
    }
    free(map);  // �� ������ �Ҵ�� �޸� ����
}

// player
Entity* createPlayer(Position start_pos)    // �÷��̾� ���� �Լ�
{
    // ��ü ����ü �� ����ŭ�� �޸� ���� �Ҵ�
    Entity* newPlayer = calloc(1, sizeof(Entity));  // ��ü ����ü ������ = �÷��̾� ����

    // �÷��̾� ��ǥ ����
    newPlayer->pos.y = start_pos.y; // �÷��̾� y��ǥ = �μ��� ���� ���� y��ǥ
    newPlayer->pos.x = start_pos.x; // �÷��̾� x��ǥ = �μ��� ���� ���� x��ǥ
    newPlayer->ch = '@';    // �÷��̾� ���ڸ� @�� ����
    newPlayer->color = COLOR_PAIR(VISIBLE_COLOR);   // �÷��̾� ������ ���� �������� ����

    return newPlayer;   // �÷��̾� ����(��ü ����ü) ��ȯ
}

void handleInput(int input) // ���� ���� �Լ�
{

    Position newPos = { player->pos.y, player->pos.x }; // �̵� ������ ��ǥ

    switch (input)  // ����ڷκ��� �Է¹��� ����
    {
    case 'w':   // w�� ���
        newPos.y--; // �̵� ������ y��ǥ ����
        break;
    case 's':   // s�� ���
        newPos.y++; // �̵� ������ y��ǥ ����
        break;
    case 'a':   // a�� ���
        newPos.x--; // �̵� ������ x��ǥ ����
        break;
    case 'd':   // d�� ���
        newPos.x++; // �̵� ������ x��ǥ ����
        break;
    default:    // w, a, s, d�� �ƴ� ���
        break;
    }

    movePlayer(newPos); // �÷��̾� �������� �̵�
}

void movePlayer(Position newPos)    // �÷��̾� �̵� �Լ�
{
    if (map[newPos.y][newPos.x].walkable)   // �̵� �������� ���� ������ ���
    {
        clearFOV(player);   // ���� �þ� ����
        player->pos.y = newPos.y;   // �÷��̾� y��ǥ�� ������ y��ǥ�� ����
        player->pos.x = newPos.x;   // �÷��̾� x��ǥ�� ������ x��ǥ�� ����
        makeFOV(player);    // �ű� �þ� ����
    }
}

// room
Room createRoom(int y, int x, int height, int width)    // �� ���� �Լ�
{
    Room newRoom;   // �� ����ü ����

    // �� ����ü ��� ����
    newRoom.pos.y = y;  // �� y��ǥ = �μ��� ���� y��ǥ
    newRoom.pos.x = x;  // �� x��ǥ = �μ��� ���� x��ǥ
    newRoom.height = height;    // �� ���� = �μ��� ���� ����
    newRoom.width = width;  // �� �ʺ� = �μ��� ���� �ʺ�
    newRoom.center.y = y + (int)(height / 2);   // �� �߽� y��ǥ = �μ��� ���� y��ǥ + �μ��� ���� ����/2
    newRoom.center.x = x + (int)(width / 2);    // �� �߽� x��ǥ = �μ��� ���� x��ǥ + �μ��� ���� �ʺ�/2

    return newRoom; // �� ����ü ���� ��ȯ
}

void addRoomToMap(Room room)    // �ʿ� �� �߰� �Լ�
{
    for (int y = room.pos.y; y < room.pos.y + room.height; y++) // �� ��� y��ǥ���� �� ���̸�ŭ �ݺ�
    {
        for (int x = room.pos.x; x < room.pos.x + room.width; x++)  // �� ���� x��ǥ���� �� �ʺ�ŭ �ݺ�
        {
            map[y][x].ch = '.'; // �� ���� ���� �� ������ ���ϴ� . ���� ����
            map[y][x].walkable = TRUE;  // ���� ���� ����
            map[y][x].transparent = TRUE;   // �þ� ��� ����
        }
    }
}

void connectRoomCenters(Position centerOne, Position centerTwo) // �� �� ���� �Լ�
{
    Position temp;  // �ӽ� ��ǥ ����ü ����
    temp.x = centerOne.x;   // �ӽ� x��ǥ = ù ��° �� �߽� x��ǥ
    temp.y = centerOne.y;   // �ӽ� y��ǥ = �� ��° �� �߽� y��ǥ

    // ù ��° ��� �� ��° �� ������ �ִܰŸ� ���� ����
    while (TRUE)
    {
        // abs(�Ǽ�) = ���� (stdlib.h)
        if (abs((temp.x - 1) - centerTwo.x) < abs(temp.x - centerTwo.x))    // �ӽ� x��ǥ ���� �� �� ��° �濡 ������� ���
            temp.x--;   // �ӽ� x��ǥ ����
        else if (abs((temp.x + 1) - centerTwo.x) < abs(temp.x - centerTwo.x))   // �ӽ� x��ǥ ���� �� �� ��° �濡 ������� ���
            temp.x++;   // �ӽ� x��ǥ ����
        else if (abs((temp.y + 1) - centerTwo.y) < abs(temp.y - centerTwo.y))   // �ӽ� y��ǥ ���� �� �� ��° �濡 ������� ���
            temp.y++;   // �ӽ� x��ǥ ����
        else if (abs((temp.y - 1) - centerTwo.y) < abs(temp.y - centerTwo.y))   // �ӽ� y��ǥ ���� �� �� ��° �濡 ������� ���
            temp.y--;   // �ӽ� x��ǥ ����
        else    // �ӽ� ��ǥ�� �� ��° �� �߽� ��ǥ�� ��ġ�� ���
            break;  // ���� ����

        map[temp.y][temp.x].ch = '.';   // �ӽ� ��ǥ�� �� ������ ���ϴ� . ���
        map[temp.y][temp.x].walkable = TRUE;    // ���� ���� ����
        map[temp.y][temp.x].transparent = TRUE; // �þ� ��� ����
    }
}

//fov
void makeFOV(Entity* player)    // ���� �þ� ���� �Լ�
{
    int y, x, distance; // ��ǥ(�ݺ��� �Ű�����), �þ� �Ǻ� ������ �Ÿ� ����
    int RADIUS = 15;    // �þ� ���� ����
    Position target;    // �þ� �Ǻ� ��� ����

    map[player->pos.y][player->pos.x].visible = TRUE;   // �÷��̾� ���� ��ǥ ���� ����
    map[player->pos.y][player->pos.x].seen = TRUE;  // �÷��̾� ���� ��ǥ �ν� ����

    for (y = player->pos.y - RADIUS; y < player->pos.y + RADIUS; y++)   // �÷��̾��� y��ǥ �þ� ������ŭ �ݺ�
    {
        for (x = player->pos.x - RADIUS; x < player->pos.x + RADIUS; x++)   // �÷��̾��� x��ǥ �þ� ������ŭ �ݺ�
        {
            target.y = y;   // �þ� �Ǻ� ��� y��ǥ = �ݺ��� �� y��ǥ
            target.x = x;   // �þ� �Ǻ� ��� x��ǥ = �ݺ��� �� x��ǥ
            distance = getDistance(player->pos, target);    // �÷��̾�� �þ� �Ǻ� ������ �Ÿ� ���

            if (distance < RADIUS)  // �÷��̾�� �þ� �Ǻ� ��� �� �Ÿ��� �þ� �������� ���� ���
            {
                if (isInMap(y, x) && lineOfSight(player->pos, target))  // �þ� �Ǻ� ����� �� ���� �ְ� ���� �þ߿� ���� ���
                {
                    map[y][x].visible = TRUE;   // �þ� �Ǻ� ��� ��ǥ ���� ����
                    map[y][x].seen = TRUE;  // �þ� �Ǻ� ��� ��ǥ �ν� ����
                }
            }
        }
    }
}

void clearFOV(Entity* player)   // �þ� ���� �Լ�
{
    int y, x;   // �ӽ� ��ǥ ����
    int RADIUS = 15;    // �þ� ���� ����

    for (y = player->pos.y - RADIUS; y < player->pos.y + RADIUS; y++)   // �÷��̾��� y��ǥ �þ� ������ŭ �ݺ�
    {
        for (x = player->pos.x - RADIUS; x < player->pos.x + RADIUS; x++)   // �÷��̾��� x��ǥ �þ� ������ŭ �ݺ�
        {
            if (isInMap(y, x))  // �þ� ���� ����� �� ���� ���� ���
                map[y][x].visible = FALSE;  // �þ� ���� ��� ��ǥ �񰡽� ����
        }
    }
}

int getDistance(Position origin, Position target)   // �÷��̾�� ��� �� �Ÿ� ��� �Լ�
{
    double dy, dx;  // ��ǥ�ະ �Ÿ� ����
    int distance;   // �÷��̾�� ��� �� �Ÿ� ����
    dx = target.x - origin.x;   // x�� �Ÿ� = ��� x��ǥ - �÷��̾� x��ǥ
    dy = target.y - origin.y;   // y�� �Ÿ� = ��� y��ǥ - �÷��̾� y��ǥ
    // floor(�Ǽ�) = �Ҽ��� ���� ���� (curses.h)
    distance = floor(sqrt((dx * dx) + (dy * dy)));  // ��Ÿ��� ������ �÷��̾�� ��� �� �Ÿ� ���

    return distance;    // �÷��̾�� ��� �� �Ÿ� ��ȯ
}

bool isInMap(int y, int x)  // ��ǥ �� ���� ���� �Ǻ� �Լ�
{
    if ((0 < y && y < MAP_HEIGHT - 1) && (0 < x && x < MAP_WIDTH - 1))  // x��ǥ�� y��ǥ�� ��� �� ���� ���� ���
    {
        return TRUE;    // TRUE ��ȯ
    }

    return FALSE;   // FALSE ��ȯ
}

bool lineOfSight(Position origin, Position target)  // ��ǥ ���� �þ� ���� �Ǻ� �Լ� (Bresenham's Line Drawing Argorithm)
{
    int t, x, y, abs_delta_x, abs_delta_y, sign_x, sign_y, delta_x, delta_y;

    delta_x = origin.x - target.x;  // �÷��̾�� �þ� �Ǻ� ��� ���� x�� �Ÿ�
    delta_y = origin.y - target.y;  // �÷��̾�� �þ� �Ǻ� ��� ���� y�� �Ÿ�

    abs_delta_x = abs(delta_x); // x�� �Ÿ� ����
    abs_delta_y = abs(delta_y); // y�� �Ÿ� ����

    sign_x = getSign(delta_x);  // x�� �Ÿ� ���� ����
    sign_y = getSign(delta_y);  // y�� �Ÿ� ���� ����

    x = target.x;   // �ӽ� x��ǥ = �þ� �Ǻ� ��� x��ǥ
    y = target.y;   // �ӽ� y��ǥ = �þ� �Ǻ� ��� y��ǥ

    if (abs_delta_x > abs_delta_y)  // x�� �Ÿ� ������ y�� �Ÿ� ���񰪺��� Ŭ ���
    {
        t = abs_delta_y * 2 - abs_delta_x;  // t(1��°) = 2dy - dx

        do
        {
            if (t >= 0) // t(k��°) >= 0 �� ����
            {
                y += sign_y;    // ���� �ȼ� ����
                t -= abs_delta_x * 2;   // t(k+1��°) = t(k��°) - 2dy - 2dx
            }

            x += sign_x;
            t += abs_delta_y * 2;   // t(k+1��°) = t(k��°) - 2dy

            if (x == origin.x && y == origin.y) // �ӽ� ��ǥ�� �÷��̾� ��ǥ�� ��ġ�� ���
            {
                return TRUE;    // ����(��) ��ȯ
            }
        } while (map[y][x].transparent);    // �ӽ� ��ǥ�� �þ� ��� ������ ��� �ݺ�

        return FALSE;   // �񰡽�(����) ��ȯ
    }
    else    // x�� �Ÿ� ������ y�� �Ÿ� ���񰪺��� ���� ��� (�� �˰����� xy�� �ٲپ� �۵�)
    {
        t = abs_delta_x * 2 - abs_delta_y;

        do
        {
            if (t >= 0)
            {
                x += sign_x;
                t -= abs_delta_y * 2;
            }

            y += sign_y;
            t += abs_delta_x * 2;

            if (x == origin.x && y == origin.y)
            {
                return TRUE;
            }
        } while (map[y][x].transparent);

        return FALSE;
    }
}

int getSign(int a)  // ���� ���� �Ǻ� �Լ�
{
    return (a < 0) ? -1 : 1;    // ���� ��� -1, �� �Ǵ� 0�� ��� 1 ��ȯ
}

// main
int main(void)
{
    Position start_pos;

    cursesSetup();  // �ʱ�ȭ
    srand(time(NULL));  // ���� �ð��� ���� �õ�� ����

    map = createMapTiles(); // �� ����
    start_pos = setupMap(); // �� ���� ���� �� ���� ��ǥ ����
    player = createPlayer(start_pos);   // ���� ��ǥ�� �÷��̾� ����

    gameLoop(); // ���� ����

    closeGame();    // ���� ����

    return 0;
}