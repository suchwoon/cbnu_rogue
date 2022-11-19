#include <curses.h> // pdcurses 라이브러리
#include <stdlib.h>
#include <time.h>

#define VISIBLE_COLOR 1 // 가시 영역 색상
#define SEEN_COLOR 2    // 인식 영역 색상

typedef struct
{
    int y;  // y 좌표
    int x;  // x 좌표
} Position;	// 좌표 구조체

typedef struct
{
    char ch;    // 타일 문자
    int color;  // 타일 색
    bool walkable;  // 타일 진입 가능 여부
    bool transparent;   // 타일 시야 통과 여부
    bool visible;   // 타일 가시 영역 여부
    bool seen;  // 타일 인식 영역 여부
} Tile; // 타일 구조체

typedef struct
{
    int height; // 방 높이
    int width;  // 방 너비
    Position pos;   // 방 좌표 (좌측 상단)
    Position center;    // 방 좌표 (중심)
} Room; // 방 구조체

typedef struct
{
    Position pos;	// 개체 좌표
    char ch;	// 개체 문자
    int color;  // 개체 색
} Entity;	// 개체 구조체

// draw 함수
void drawMap(void); // 맵 출력 함수
void drawEntity(Entity* entity);    // 개체 출력 함수
void drawEverything(void);  // 화면 출력 함수

// engine 함수
void cursesSetup(void); // 초기화 함수
void gameLoop(void);    // 턴 루프 함수
void closeGame(void);   // 게임 종료 함수

// map 함수
Tile** createMapTiles(void);    // 맵 생성 함수
Position setupMap(void);    // 방 정보 설정 함수
void freeMap(void); // 맵 변수 메모리 해제 함수

// player 함수
Entity* createPlayer(Position start_pos);   // 플레이어 생성 함수
void handleInput(int input);    // 조작 적용 함수
void movePlayer(Position newPos);   // 플레이어 이동 함수

// room 함수
Room createRoom(int y, int x, int height, int width);   // 방 생성 함수
void addRoomToMap(Room room);   // 맵에 방 추가 함수
void connectRoomCenters(Position centerOne, Position centerTwo);    // 두 방 연결 함수

// fov 함수
void makeFOV(Entity* player);   // 현재 시야 생성 함수
void clearFOV(Entity * player);
int getDistance(Position origin, Position target);  // 플레이어와 대상 간 거리 계산 함수
bool isInMap(int y, int x); // 좌표 맵 내부 여부 판별 함수
bool lineOfSight(Position origin, Position target); // 좌표 직선 시야 여부 판별 함수 (Bresenham's Line Drawing Algorithm)
int getSign(int a); // 정수 음양 판별 함수

// 전역변수 & 정적변수(const)
const int MAP_HEIGHT = 25;  // 맵 높이
const int MAP_WIDTH = 100;  // 맵 너비

// 전역변수
Entity* player; // 플레이어 (개체 구조체 포인터)
Tile** map; // 맵 (타일 구조체 이중 포인터 = 타일 구조체 2차원 배열)

// draw
void drawMap(void)  // 맵 출력 함수
{
    for (int y = 0; y < MAP_HEIGHT; y++)    // 맵 높이만큼 반복
    {
        for (int x = 0; x < MAP_WIDTH; x++) // 맵 너비만큼 반복
        {
            if (map[y][x].visible)  // 해당 좌표가 가시(시야 범위 내) 타일일 경우
            {
                // mvaddch(y좌표, x좌표, 문자 | 색상); (curses.h)
                mvaddch(y, x, map[y][x].ch | map[y][x].color);  // 해당 타일에 저장된 문자(저장된 색으로) 출력
            }
            else if (map[y][x].seen)    // 해당 좌표가 가시 타일이 아니고 인식(시야 범위 내에 한 번이라도 들어온) 타일일 경우
            {
                mvaddch(y, x, map[y][x].ch | COLOR_PAIR(SEEN_COLOR));   // 해당 타일에 저장된 문자(인식 색으로) 출력
            }
            else    // 해당 좌표가 한 번도 시야 범위에 들어온 적 없는 타일일 경우
            {
                mvaddch(y, x, ' '); // 공백으로 출력
            }
        }
    }
}

void drawEntity(Entity* entity) // 개체 출력 함수
{
    mvaddch(entity->pos.y, entity->pos.x, entity->ch | entity->color);  // 개체 좌표에 개체 문자(개체 색상으로) 출력
}

void drawEverything(void)   // 화면 출력 함수
{
    clear();    // 콘솔 창 초기화
    drawMap();  // 맵 출력
    drawEntity(player); // 개체 출력
}

// engine
void cursesSetup(void)  // 초기화 함수
{
    initscr();  // curses 모드 시작
    noecho();   // 사용자로부터 입력받은 문자 출력 X
    curs_set(0);    // 커서 보이지 않게

    if (has_colors())   // 콘솔이 색상 출력 가능할 경우
    {
        start_color();  // color 모드 시작

        // init_pair(팔레트 이름, 글자색, 배경색); (curses.h)
        init_pair(VISIBLE_COLOR, COLOR_WHITE, COLOR_BLACK); // 가시 영역 팔레트 설정
        init_pair(SEEN_COLOR, COLOR_BLUE, COLOR_BLACK); // 인식 영역 팔레트 설정
    }
}

void gameLoop(void) // 턴 루프 함수
{
    int ch; // 사용자 입력 문자 변수

    makeFOV(player);    // 초기 시야 생성
    drawEverything();   // 화면 출력

    while (ch = getch())    // 문자를 입력받았을 경우
    {
        if (ch == 'q')  // q를 입력받았을 경우
        {
            break;  // 게임 종료
        }

        handleInput(ch);    // 조작 적용 함수에 입력 문자 전달
        drawEverything();   // 화면 출력
    }
}

void closeGame(void)    // 게임 종료 함수
{
    endwin();   // curses 모드 종료
    free(player);   // 플레이어 변수(개체 구조체 포인터) 메모리 해제
}

// map
Tile** createMapTiles(void) // 맵 생성 함수
{
    // 타일 구조체 포인터(배열)의 크기 * 맵 높이만큼의 메모리 동적 할당
    Tile** tiles = calloc(MAP_HEIGHT, sizeof(Tile*));   // 타일 이중 포인터(2차원 배열) = 맵 변수

    for (int y = 0; y < MAP_HEIGHT; y++)    // 맵 높이만큼 반복
    {
        // 타일 구조체의 크기 * 맵 너비만큼의 메모리 동적 할당
        tiles[y] = calloc(MAP_WIDTH, sizeof(Tile)); // 각 타일 배열 (맵 각 행을 의미)
        for (int x = 0; x < MAP_WIDTH; x++) // 맵 너비만큼 반복
        {
            tiles[y][x].ch = '#';   // 모든 타일 문자를 벽을 나타내는 #으로 설정
            tiles[y][x].color = COLOR_PAIR(VISIBLE_COLOR);  // 가시 색상 설정
            tiles[y][x].walkable = FALSE;   // 진입 불가 설정
            tiles[y][x].transparent = FALSE;    // 불투명 설정
            tiles[y][x].visible = FALSE;    // 불가시 설정
            tiles[y][x].seen = FALSE;   // 비인식 설정
        }
    }

    return tiles;   // 맵 변수(타일 구조체 이중 포인터) 반환
}

Position setupMap(void) // 방 정보 설정 함수
{
    int y, x, height, width, n_rooms;   // 방 y좌표, 방 x좌표, 방 높이, 방 너비, 방 개수
    n_rooms = (rand() % 11) + 5;    // 방 개수 5~15개 중 랜덤으로 설정
    // 방 구조체의 크기 * 방 개수만큼의 메모리 동적 할당
    Room* rooms = calloc(n_rooms, sizeof(Room));    // 방 포인터(배열) (모든 방 구조체의 정보 저장)
    Position start_pos; // 플레이어 시작 좌표 변수

    for (int i = 0; i < n_rooms; i++)   // 방 개수만큼 반복
    {
        y = (rand() % (MAP_HEIGHT - 10)) + 1;   // 방 y좌표 1~15 중 랜덤으로 설정
        x = (rand() % (MAP_WIDTH - 20)) + 1;    // 방 x좌표 1~80 중 랜덤으로 설정
        height = (rand() % 7) + 3;  // 방 높이 3~9 중 랜덤으로 설정
        width = (rand() % 15) + 5;  // 방 너비 5~19 중 랜덤으로 설정
        rooms[i] = createRoom(y, x, height, width); // 방 생성 함수에서 반환받은 구조체를 방 변수에 저장
        addRoomToMap(rooms[i]); // 맵에 방 추가

        if (i > 0)  // i가 0 초과일 경우 = 방이 2개 이상 생성되었을 경우
        {
            connectRoomCenters(rooms[i - 1].center, rooms[i].center);   // 생성한 방과 직전 생성한 방 연결
        }
    }

    // 플레이어 시작 좌표 설정
    start_pos.y = rooms[0].center.y;    // 플레이어 시작 y좌표 = 첫 번째 생성된 방의 중심 y좌표
    start_pos.x = rooms[0].center.x;    // 플레이어 시작 x좌표 = 첫 번째 생성된 방의 중심 x좌표

    free(rooms);    // 방 변수 메모리 해제

    return start_pos;   // 플레이어 시작 좌표(좌표 구조체) 반환
}

void freeMap(void)  // 맵 변수 메모리 해제 함수
{
    for (int y = 0; y < MAP_HEIGHT; y++)    // 맵 높이만큼 반복
    {
        free(map[y]);   // 각 맵 행(타일 배열)에 할당된 메모리 해제
    }
    free(map);  // 맵 변수에 할당된 메모리 해제
}

// player
Entity* createPlayer(Position start_pos)    // 플레이어 생성 함수
{
    // 개체 구조체 한 개만큼의 메모리 동적 할당
    Entity* newPlayer = calloc(1, sizeof(Entity));  // 개체 구조체 포인터 = 플레이어 변수

    // 플레이어 좌표 설정
    newPlayer->pos.y = start_pos.y; // 플레이어 y좌표 = 인수로 받은 시작 y좌표
    newPlayer->pos.x = start_pos.x; // 플레이어 x좌표 = 인수로 받은 시작 x좌표
    newPlayer->ch = '@';    // 플레이어 문자를 @로 설정
    newPlayer->color = COLOR_PAIR(VISIBLE_COLOR);   // 플레이어 색상을 가시 색상으로 설정

    return newPlayer;   // 플레이어 변수(개체 구조체) 반환
}

void handleInput(int input) // 조작 적용 함수
{

    Position newPos = { player->pos.y, player->pos.x }; // 이동 목적지 좌표

    switch (input)  // 사용자로부터 입력받은 값이
    {
    case 'w':   // w일 경우
        newPos.y--; // 이동 목적지 y좌표 감소
        break;
    case 's':   // s일 경우
        newPos.y++; // 이동 목적지 y좌표 증가
        break;
    case 'a':   // a일 경우
        newPos.x--; // 이동 목적지 x좌표 감소
        break;
    case 'd':   // d일 경우
        newPos.x++; // 이동 목적지 x좌표 증가
        break;
    default:    // w, a, s, d가 아닐 경우
        break;
    }

    movePlayer(newPos); // 플레이어 목적지로 이동
}

void movePlayer(Position newPos)    // 플레이어 이동 함수
{
    if (map[newPos.y][newPos.x].walkable)   // 이동 목적지가 진입 가능할 경우
    {
        clearFOV(player);   // 기존 시야 제거
        player->pos.y = newPos.y;   // 플레이어 y좌표를 목적지 y좌표로 설정
        player->pos.x = newPos.x;   // 플레이어 x좌표를 목적지 x좌표로 설정
        makeFOV(player);    // 신규 시야 생성
    }
}

// room
Room createRoom(int y, int x, int height, int width)    // 방 생성 함수
{
    Room newRoom;   // 방 구조체 변수

    // 방 구조체 요소 설정
    newRoom.pos.y = y;  // 방 y좌표 = 인수로 받은 y좌표
    newRoom.pos.x = x;  // 방 x좌표 = 인수로 받은 x좌표
    newRoom.height = height;    // 방 높이 = 인수로 받은 높이
    newRoom.width = width;  // 방 너비 = 인수로 받은 너비
    newRoom.center.y = y + (int)(height / 2);   // 방 중심 y좌표 = 인수로 받은 y좌표 + 인수로 받은 높이/2
    newRoom.center.x = x + (int)(width / 2);    // 방 중심 x좌표 = 인수로 받은 x좌표 + 인수로 받은 너비/2

    return newRoom; // 방 구조체 변수 반환
}

void addRoomToMap(Room room)    // 맵에 방 추가 함수
{
    for (int y = room.pos.y; y < room.pos.y + room.height; y++) // 방 상단 y좌표부터 방 높이만큼 반복
    {
        for (int x = room.pos.x; x < room.pos.x + room.width; x++)  // 방 좌측 x좌표부터 방 너비만큼 반복
        {
            map[y][x].ch = '.'; // 방 내부 문자 빈 공간을 뜻하는 . 으로 설정
            map[y][x].walkable = TRUE;  // 진입 가능 설정
            map[y][x].transparent = TRUE;   // 시야 통과 설정
        }
    }
}

void connectRoomCenters(Position centerOne, Position centerTwo) // 두 방 연결 함수
{
    Position temp;  // 임시 좌표 구조체 변수
    temp.x = centerOne.x;   // 임시 x좌표 = 첫 번째 방 중심 x좌표
    temp.y = centerOne.y;   // 임시 y좌표 = 두 번째 방 중심 y좌표

    // 첫 번째 방과 두 번째 방 사이의 최단거리 도출 과정
    while (TRUE)
    {
        // abs(실수) = 절댓값 (stdlib.h)
        if (abs((temp.x - 1) - centerTwo.x) < abs(temp.x - centerTwo.x))    // 임시 x좌표 감소 시 두 번째 방에 가까워질 경우
            temp.x--;   // 임시 x좌표 감소
        else if (abs((temp.x + 1) - centerTwo.x) < abs(temp.x - centerTwo.x))   // 임시 x좌표 증가 시 두 번째 방에 가까워질 경우
            temp.x++;   // 임시 x좌표 감소
        else if (abs((temp.y + 1) - centerTwo.y) < abs(temp.y - centerTwo.y))   // 임시 y좌표 감소 시 두 번째 방에 가까워질 경우
            temp.y++;   // 임시 x좌표 감소
        else if (abs((temp.y - 1) - centerTwo.y) < abs(temp.y - centerTwo.y))   // 임시 y좌표 증가 시 두 번째 방에 가까워질 경우
            temp.y--;   // 임시 x좌표 감소
        else    // 임시 좌표가 두 번째 방 중심 좌표와 일치할 경우
            break;  // 루프 종료

        map[temp.y][temp.x].ch = '.';   // 임시 좌표에 빈 공간을 뜻하는 . 출력
        map[temp.y][temp.x].walkable = TRUE;    // 진입 가능 설정
        map[temp.y][temp.x].transparent = TRUE; // 시야 통과 설정
    }
}

//fov
void makeFOV(Entity* player)    // 현재 시야 생성 함수
{
    int y, x, distance; // 좌표(반복문 매개변수), 시야 판별 대상과의 거리 변수
    int RADIUS = 15;    // 시야 범위 변수
    Position target;    // 시야 판별 대상 변수

    map[player->pos.y][player->pos.x].visible = TRUE;   // 플레이어 현재 좌표 가시 설정
    map[player->pos.y][player->pos.x].seen = TRUE;  // 플레이어 현재 좌표 인식 설정

    for (y = player->pos.y - RADIUS; y < player->pos.y + RADIUS; y++)   // 플레이어의 y좌표 시야 범위만큼 반복
    {
        for (x = player->pos.x - RADIUS; x < player->pos.x + RADIUS; x++)   // 플레이어의 x좌표 시야 범위만큼 반복
        {
            target.y = y;   // 시야 판별 대상 y좌표 = 반복문 내 y좌표
            target.x = x;   // 시야 판별 대상 x좌표 = 반복문 내 x좌표
            distance = getDistance(player->pos, target);    // 플레이어와 시야 판별 대상과의 거리 계산

            if (distance < RADIUS)  // 플레이어와 시야 판별 대상 간 거리가 시야 범위보다 작을 경우
            {
                if (isInMap(y, x) && lineOfSight(player->pos, target))  // 시야 판별 대상이 맵 내에 있고 직선 시야에 있을 경우
                {
                    map[y][x].visible = TRUE;   // 시야 판별 대상 좌표 가시 설정
                    map[y][x].seen = TRUE;  // 시야 판별 대상 좌표 인식 설정
                }
            }
        }
    }
}

void clearFOV(Entity* player)   // 시야 제거 함수
{
    int y, x;   // 임시 좌표 변수
    int RADIUS = 15;    // 시야 범위 변수

    for (y = player->pos.y - RADIUS; y < player->pos.y + RADIUS; y++)   // 플레이어의 y좌표 시야 범위만큼 반복
    {
        for (x = player->pos.x - RADIUS; x < player->pos.x + RADIUS; x++)   // 플레이어의 x좌표 시야 범위만큼 반복
        {
            if (isInMap(y, x))  // 시야 제거 대상이 맵 내에 있을 경우
                map[y][x].visible = FALSE;  // 시야 제거 대상 좌표 비가시 설정
        }
    }
}

int getDistance(Position origin, Position target)   // 플레이어와 대상 간 거리 계산 함수
{
    double dy, dx;  // 좌표축별 거리 변수
    int distance;   // 플레이어와 대상 간 거리 변수
    dx = target.x - origin.x;   // x축 거리 = 대상 x좌표 - 플레이어 x좌표
    dy = target.y - origin.y;   // y축 거리 = 대상 y좌표 - 플레이어 y좌표
    // floor(실수) = 소숫점 이하 버림 (curses.h)
    distance = floor(sqrt((dx * dx) + (dy * dy)));  // 피타고라스 정리로 플레이어와 대상 간 거리 계산

    return distance;    // 플레이어와 대상 간 거리 반환
}

bool isInMap(int y, int x)  // 좌표 맵 내부 여부 판별 함수
{
    if ((0 < y && y < MAP_HEIGHT - 1) && (0 < x && x < MAP_WIDTH - 1))  // x좌표와 y좌표가 모두 맵 범위 내일 경우
    {
        return TRUE;    // TRUE 반환
    }

    return FALSE;   // FALSE 반환
}

bool lineOfSight(Position origin, Position target)  // 좌표 직선 시야 여부 판별 함수 (Bresenham's Line Drawing Argorithm)
{
    int t, x, y, abs_delta_x, abs_delta_y, sign_x, sign_y, delta_x, delta_y;

    delta_x = origin.x - target.x;  // 플레이어와 시야 판별 대상 사이 x축 거리
    delta_y = origin.y - target.y;  // 플레이어와 시야 판별 대상 사이 y축 거리

    abs_delta_x = abs(delta_x); // x축 거리 절댓값
    abs_delta_y = abs(delta_y); // y축 거리 절댓값

    sign_x = getSign(delta_x);  // x축 거리 음양 여부
    sign_y = getSign(delta_y);  // y축 거리 음양 여부

    x = target.x;   // 임시 x좌표 = 시야 판별 대상 x좌표
    y = target.y;   // 임시 y좌표 = 시야 판별 대상 y좌표

    if (abs_delta_x > abs_delta_y)  // x축 거리 절댓값이 y축 거리 절댓값보다 클 경우
    {
        t = abs_delta_y * 2 - abs_delta_x;  // t(1번째) = 2dy - dx

        do
        {
            if (t >= 0) // t(k번째) >= 0 일 때만
            {
                y += sign_y;    // 선택 픽셀 변경
                t -= abs_delta_x * 2;   // t(k+1번째) = t(k번째) - 2dy - 2dx
            }

            x += sign_x;
            t += abs_delta_y * 2;   // t(k+1번째) = t(k번째) - 2dy

            if (x == origin.x && y == origin.y) // 임시 좌표가 플레이어 좌표와 일치할 경우
            {
                return TRUE;    // 가시(참) 반환
            }
        } while (map[y][x].transparent);    // 임시 좌표가 시야 통과 가능할 경우 반복

        return FALSE;   // 비가시(거짓) 반환
    }
    else    // x축 거리 절댓값이 y축 거리 절댓값보다 작을 경우 (위 알고리즘을 xy축 바꾸어 작동)
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

int getSign(int a)  // 정수 음양 판별 함수
{
    return (a < 0) ? -1 : 1;    // 음일 경우 -1, 양 또는 0일 경우 1 반환
}

// main
int main(void)
{
    Position start_pos;

    cursesSetup();  // 초기화
    srand(time(NULL));  // 현재 시각을 난수 시드로 설정

    map = createMapTiles(); // 맵 생성
    start_pos = setupMap(); // 방 정보 설정 및 시작 좌표 설정
    player = createPlayer(start_pos);   // 시작 좌표에 플레이어 생성

    gameLoop(); // 게임 실행

    closeGame();    // 게임 종료

    return 0;
}