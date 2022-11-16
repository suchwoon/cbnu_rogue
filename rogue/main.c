#include <curses.h>
#include <stdlib.h>
#include <time.h>

typedef struct
{
    int y;  // y 좌표
    int x;  // x 좌표
} Position;	// 좌표 구조체

typedef struct
{
    char ch;    // 타일 문자
    bool walkable;  // 진입 가능 여부
} Tile; // 타일 구조체

typedef struct
{
    int height;
    int width;
    Position pos;
    Position center;
} Room; // 방 구조체

typedef struct
{
    Position pos;	// 개체 좌표
    char ch;	// 개체 문자
} Entity;	// 개체 구조체

// draw 함수
void drawMap(void);
void drawEntity(Entity* entity);
void drawEverything(void);

// engine 함수
void cursesSetup(void);
void gameLoop(void);
void closeGame(void);

// map 함수
Tile** createMapTiles(void);
Position setupMap(void);
void freeMap(void);

// player 함수
Entity* createPlayer(Position start_pos);
void handleInput(int input);
void movePlayer(Position newPos);

// room 함수
Room createRoom(int y, int x, int height, int width);
void addRoomToMap(Room room);
void connectRoomCenters(Position centerOne, Position centerTwo);

const int MAP_HEIGHT = 25;
const int MAP_WIDTH = 100;

Entity* player;
Tile** map;

// draw
void drawMap(void)
{
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            mvaddch(y, x, map[y][x].ch);
        }
    }
}

void drawEntity(Entity* entity)
{
    mvaddch(entity->pos.y, entity->pos.x, entity->ch);
}

void drawEverything(void)
{
    clear();
    drawMap();
    drawEntity(player);
}

// engine
void cursesSetup(void)
{
    initscr();
    noecho();
    curs_set(0);
}

void gameLoop(void)
{
    int ch;

    drawEverything();

    while (ch = getch())
    {
        if (ch == 'q')
        {
            break;
        }

        handleInput(ch);
        drawEverything();
    }
}

void closeGame(void)
{
    endwin();
    free(player);
}

// map
Tile** createMapTiles(void)
{
    Tile** tiles = calloc(MAP_HEIGHT, sizeof(Tile*));

    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        tiles[y] = calloc(MAP_WIDTH, sizeof(Tile));
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            tiles[y][x].ch = '#';
            tiles[y][x].walkable = FALSE;
        }
    }

    return tiles;
}

Position setupMap(void)
{
    int y, x, height, width, n_rooms;
    n_rooms = (rand() % 11) + 5;
    Room* rooms = calloc(n_rooms, sizeof(Room));
    Position start_pos;

    for (int i = 0; i < n_rooms; i++)
    {
        y = (rand() % (MAP_HEIGHT - 10)) + 1;
        x = (rand() % (MAP_WIDTH - 20)) + 1;
        height = (rand() % 7) + 3;
        width = (rand() % 15) + 5;
        rooms[i] = createRoom(y, x, height, width);
        addRoomToMap(rooms[i]);

        if (i > 0)
        {
            connectRoomCenters(rooms[i - 1].center, rooms[i].center);
        }
    }

    start_pos.y = rooms[0].center.y;
    start_pos.x = rooms[0].center.x;

    free(rooms);

    return start_pos;
}

void freeMap(void)
{
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        free(map[y]);
    }
    free(map);
}

// player
Entity* createPlayer(Position start_pos)
{
    Entity* newPlayer = calloc(1, sizeof(Entity));

    newPlayer->pos.y = start_pos.y;
    newPlayer->pos.x = start_pos.x;
    newPlayer->ch = '@';

    return newPlayer;
}

void handleInput(int input)
{

    Position newPos = { player->pos.y, player->pos.x }; // 이동 목적지 좌표

    switch (input)
    {
        //move up
    case 'k':
        newPos.y--;
        break;
        //move down
    case 'j':
        newPos.y++;
        break;
        //move left
    case 'h':
        newPos.x--;
        break;
        //move right
    case 'l':
        newPos.x++;
        break;
    default:
        break;
    }

    movePlayer(newPos);
}

void movePlayer(Position newPos)    // 목적지 이동 함수
{
    if (map[newPos.y][newPos.x].walkable)   // 이동 목적지가 진입 가능하면
    {
        player->pos.y = newPos.y;
        player->pos.x = newPos.x;
    }
}

// room
Room createRoom(int y, int x, int height, int width)
{
    Room newRoom;

    newRoom.pos.y = y;
    newRoom.pos.x = x;
    newRoom.height = height;
    newRoom.width = width;
    newRoom.center.y = y + (int)(height / 2);
    newRoom.center.x = x + (int)(width / 2);

    return newRoom;
}

void addRoomToMap(Room room)
{
    for (int y = room.pos.y; y < room.pos.y + room.height; y++)
    {
        for (int x = room.pos.x; x < room.pos.x + room.width; x++)
        {
            map[y][x].ch = '.';
            map[y][x].walkable = TRUE;
        }
    }
}

void connectRoomCenters(Position centerOne, Position centerTwo)
{
    Position temp;
    temp.x = centerOne.x;
    temp.y = centerOne.y;

    while (TRUE)
    {
        if (abs((temp.x - 1) - centerTwo.x) < abs(temp.x - centerTwo.x))
            temp.x--;
        else if (abs((temp.x + 1) - centerTwo.x) < abs(temp.x - centerTwo.x))
            temp.x++;
        else if (abs((temp.y + 1) - centerTwo.y) < abs(temp.y - centerTwo.y))
            temp.y++;
        else if (abs((temp.y - 1) - centerTwo.y) < abs(temp.y - centerTwo.y))
            temp.y--;
        else
            break;

        map[temp.y][temp.x].ch = '.';
        map[temp.y][temp.x].walkable = TRUE;
    }
}

// main
int main(void)
{
    Position start_pos;

    cursesSetup();
    srand(time(NULL));

    map = createMapTiles();
    start_pos = setupMap();
    player = createPlayer(start_pos);

    gameLoop();

    closeGame();

    return 0;
}