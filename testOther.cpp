#include <iostream>
#include <windows.h>
#include <string>
#include <graphics.h>
#include <vector>

const int windowWidth = 1280;
const int windowHeight = 720;
const int frameInterval = 1000 / 120;

bool is_game_started = false;
bool running = true;

const int BUTTON_WIDTH = 192;
const int BUTTON_HEIGHT = 75;

#pragma comment(lib, "MSIMG32.LIB")
#pragma comment(lib, "Winmm.lib")


class Animation {
private:
	std::vector<IMAGE*> imgs;
	int imgIndex = 0;

	int frameInterval = 0;								//帧间隔ms: 两帧动画间的时间

	int timer = 0;										//计时器: 本张动画已经播放的时间

public:
	Animation(LPCTSTR path, int num, int frameInterval);//LPCSTR 更通用的常字符指针
	~Animation();
public:
	//播放一张动画
	void play(int x, int y, int playTime);				//playTime :本张动画播放的时间
};

class Player {
private:
	POINT playerPos{ 500,500 };							//玩家位置

	const int playerSpeed = 6;							//移动速度

	bool isLeft = false, isRight = false,				//移动方向
		isUp = false, isDown = false;

	bool isFacingLeft = false;							//面部朝向

	const int shadowWidth = 32;							//玩家阴影高度

public:
	const int width = 80;								//玩家高度
	const int height = 80;

private:
	IMAGE playerShadow;
	Animation* animationPlayerLeft;						//玩家动画
	Animation* animationPlayerRight;
public:
	Player();
	~Player();
public:
	void processMessage(const ExMessage& msg);

	void move();
	void draw(int frameInterval);
public:
	int x() const { return playerPos.x; }
	int y() const { return playerPos.y; }
};

class Bullet {
private:
	const int radius = 10;
public:
	POINT pos{ 0,0 };
public:
	void draw() const {
		//橙红色填充圆
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));

		fillcircle(pos.x, pos.y, radius);
	}
};

class Enemy {
private:
	POINT pos{ 0,0 };									//敌人位置

	const int width = 80;								//敌人高度
	const int height = 80;
	const int shadowWidth = 48;							//敌人阴影高度

	const int enemySpeed = 2;							//移动速度

	bool isLeft = false, isRight = false,				//移动方向
		isUp = false, isDown = false;

	bool isFacingLeft = false;							//面部朝向	

	bool isAlive = true;									//怪物存活

private:
	IMAGE enemyShadow;
	Animation* animationEnemyLeft;						//敌人动画
	Animation* animationEnemyRight;
public:
	Enemy();
	~Enemy();
public:
	void move(const Player& player);

	bool checkBulletCollision(const Bullet& bullet) const;
	bool checkPlayerCollision(const Player& player) const;

	void draw(int frameInterval);
public:
	void hurt() { isAlive = false; }
	bool checkAlive() const { return isAlive; }
};

void putImageAlpha(int x, int y, IMAGE* img);								//图像绘制(透明度)
void generateEnemy(std::vector<Enemy* >& enemys);							//生成敌人
void updateBullets(std::vector<Bullet>& bullets, const Player& player);		//更新子弹
void drawScore(const int& score);

//按钮类
class Button {
public:
	//const int BUTTON_WIDTH
	Button(RECT rect ,LPCTSTR path_img_idle,LPCTSTR path_img_hovered,LPCTSTR path_img_pushed) {

		region = rect;
		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushed, path_img_pushed);

	}

	~Button() = default;
	
	void ProcessEvent(const ExMessage& msg) {
		switch (msg.message) {
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))
				status = Status::Hovered;
			else if (status == Status::Hovered && !CheckCursorHit(msg.x,msg.y))
				status = Status::Idle;
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursorHit(msg.x, msg.y))
				status = Status::Pushed;
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed)
				OnClick();
			break;
		default:
			break;
		}
	}

	void Draw() {
		switch (status) {
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_idle);
			break;

		}


	}

protected:
	virtual void OnClick() = 0;
private:
	enum class Status {
		Idle = 0,
		Hovered,
		Pushed
	};

//检测鼠标点击
	bool CheckCursorHit(int x, int y) {
		return x >= region.left <= region.right && y >= region.top && y <= region.bottom;
	}

private:
	RECT region;
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status = Status::Idle;
};

//开始游戏按钮
class StartGameButton :public Button {
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~StartGameButton() = default;

protected:
	void OnClick() {
		is_game_started = true;
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}
};

//退出游戏按钮
class QuitGameButton :public Button {
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {
	}
	~QuitGameButton() = default;

protected:
	void OnClick() {
		running = false;
	}
};



int main() {
	initgraph(windowWidth, windowHeight);

	//加载地图
	IMAGE background;
	IMAGE img_menu;;
	

	//加载mp3 取alias bgm
	mciSendString(_T("open mus/bgm.mp3 alias bgm"), nullptr, 0, nullptr);
	mciSendString(_T("open mus/hit.wav alias hit"), nullptr, 0, nullptr);
	//重复播放bgm 从0开始
	mciSendString(_T("play bgm repeat from 0"), nullptr, 0, nullptr);

	Player player;
	std::vector<Bullet> bullets(3);
	std::vector<Enemy*> enemys;

	RECT region_btn_start_game, region_btn_quit_game;
	region_btn_start_game.left = (windowWidth - BUTTON_WIDTH) / 2;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;

	region_btn_quit_game.left = (windowWidth - BUTTON_WIDTH) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
	region_btn_quit_game.top = 550;
	region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;

	StartGameButton btn_start_game = StartGameButton(region_btn_start_game, _T("img/ui_start_idle.png")
		, _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));
	QuitGameButton btn_quit_game = QuitGameButton(region_btn_quit_game, _T("img/ui_quit_idle.png")
		, _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

	loadimage(&img_menu, _T("img/menu.png"));
	loadimage(&background, _T("img/background.png"));
	unsigned int score = 0;

	//bool running = true;
	ExMessage message;

	BeginBatchDraw();
	while (running) {
		ULONGLONG startTime = GetTickCount64();

		//读数据
		peekmessage(&message);

		//处理数据
		if(is_game_started)
			player.processMessage(message);
		else
		{
			//btn_start_game.ProcessEvent(msg);
			btn_start_game.ProcessEvent(message);
			btn_quit_game.ProcessEvent(message);
		}

		if (is_game_started) {
			//更新玩家
			player.move();

			//更新子弹
			updateBullets(bullets, player);

			generateEnemy(enemys);
			for (auto& enemy : enemys)
				enemy->move(player);

			//怪物和子弹碰撞检测
			for (auto& enemy : enemys)
				for (const auto& bullet : bullets)
					if (enemy->checkBulletCollision(bullet)) {
						score++;
						enemy->hurt();
						mciSendString(_T("play hit from 0"), nullptr, 0, nullptr);
					}

			//怪物和玩家碰撞检测
			for (auto& enemy : enemys) {
				if (enemy->checkPlayerCollision(player)) {
					TCHAR text[64];
					_stprintf_s(text, _T("最终得分: %d"), score);
					MessageBox(GetHWnd(), text, _T("游戏结束"), MB_OK);

					running = false;
					break;
				}
			}

			//移除以消失的怪物
			for (auto& enemy : enemys) {

				for (const auto& bullet : bullets) {

					if (!enemy->checkAlive()) {

						std::swap(enemy, enemys.back());
						delete enemys.back();
						enemys.pop_back();
					}

				}
			}
		}
		

		//渲染
		cleardevice();
		if (is_game_started) {
			putimage(0, 0, &background);

			player.draw(frameInterval);
			for (const auto& bullet : bullets)
				bullet.draw();
			for (const auto& enemy : enemys)
				enemy->draw(frameInterval);

			drawScore(score);
		}
		else {
			putimage(0, 0, &img_menu);
			btn_start_game.Draw();
			btn_quit_game.Draw();
		}
		
		FlushBatchDraw();
		//120刷新
		ULONGLONG executionTime = GetTickCount64() - startTime;
		if (executionTime < frameInterval)
			Sleep(frameInterval - executionTime);
	}
	EndBatchDraw();
	for (auto& enemy : enemys)
		delete enemy;
}

Player::Player() {

	loadimage(&playerShadow, _T("img/shadow_player.png"));

	animationPlayerLeft = new Animation(_T("img/player_left_%d.png"), 6, 45);
	animationPlayerRight = new Animation(_T("img/player_right_%d.png"), 6, 45);
}

Player::~Player() {
	delete animationPlayerLeft;
	delete animationPlayerRight;
}

void Player::processMessage(const ExMessage& msg) {

	//判断移动方向
	if (msg.message == WM_KEYDOWN) {

		switch (msg.vkcode) {

		case VK_UP:
			isUp = true;
			break;
		case VK_DOWN:
			isDown = true;
			break;
		case VK_LEFT:
			isLeft = true;
			break;
		case VK_RIGHT:
			isRight = true;
			break;
		default:
			break;
		}
	}
	else if (msg.message == WM_KEYUP) {
		switch (msg.vkcode) {

		case VK_UP:
			isUp = false;
			break;
		case VK_DOWN:
			isDown = false;
			break;
		case VK_LEFT:
			isLeft = false;
			break;
		case VK_RIGHT:
			isRight = false;
			break;
		default:
			break;
		}

	}
}

//计算移动信息
void Player::move() {

	// x,y 代表 向量
	int x = isRight - isLeft;
	int y = isDown - isUp;

	double modulus = sqrt(x * x + y * y);	//向量的模

	if (modulus) {

		double vectorX = x / modulus;
		double vectorY = y / modulus;

		playerPos.x += int(playerSpeed * vectorX);
		playerPos.y += int(playerSpeed * vectorY);
	}

	//校准
	if (playerPos.x < 0)	playerPos.x = 0;
	if (playerPos.y < 0)	playerPos.y = 0;

	if (playerPos.x + width > windowWidth)	playerPos.x = windowWidth - width;
	if (playerPos.y + height > windowHeight)	playerPos.y = windowHeight - height;

	//修改面部朝向
		//等于0时，指向原先面部朝向
	if (x > 0)
		isFacingLeft = false;
	else if (x < 0)
		isFacingLeft = true;

}

void Player::draw(int frameInterval) {

	//绘制阴影
	int xShadow = playerPos.x + (width - shadowWidth) / 2;
	int yShadow = playerPos.y + height - 8;
	putImageAlpha(xShadow, yShadow, &playerShadow);

	//绘制动画
	if (isFacingLeft)
		animationPlayerLeft->play(playerPos.x, playerPos.y, frameInterval);
	else
		animationPlayerRight->play(playerPos.x, playerPos.y, frameInterval);
}

Enemy::Enemy() {

	loadimage(&enemyShadow, _T("img/shadow_enemy.png"));

	animationEnemyLeft = new Animation(_T("img/enemy_left_%d.png"), 6, 45);
	animationEnemyRight = new Animation(_T("img/enemy_right_%d.png"), 6, 45);

	enum spawnEdge { up, down, left, right };
	spawnEdge edge = spawnEdge(rand() % 4);

	switch (edge) {

	case up:
		pos.x = rand() % windowWidth;
		pos.y = -height;
		break;
	case down:
		pos.x = rand() % windowWidth;
		pos.y = windowHeight;
		break;
	case left:
		pos.x = -width;
		pos.y = rand() % windowHeight;
		break;
	case right:
		pos.x = windowWidth;
		pos.y = rand() % windowHeight;
		break;
	default:
		break;
	}


}

Enemy::~Enemy() {
	delete animationEnemyLeft;
	delete animationEnemyRight;
}

void Enemy::move(const Player& player) {

	//怪物向玩家移动
	int x = player.x() - pos.x;
	int y = player.y() - pos.y;

	double modulus = sqrt(x * x + y * y);	//向量的模

	if (modulus) {

		double vectorX = x / modulus;
		double vectorY = y / modulus;

		pos.x += int(enemySpeed * vectorX);
		pos.y += int(enemySpeed * vectorY);
	}

	//修改面部朝向
	if (x > 0)
		isFacingLeft = false;
	else if (x < 0)
		isFacingLeft = true;
}

bool Enemy::checkBulletCollision(const Bullet& bullet) const {

	//判断子弹 是否在 矩形内
	bool isInX = bullet.pos.x >= pos.x && bullet.pos.x <= pos.x + width;

	bool isInY = bullet.pos.y >= pos.y && bullet.pos.y <= pos.y + height;

	return isInX && isInY;
}

bool Enemy::checkPlayerCollision(const Player& player) const {
	//判断中心点 是否在 玩家矩形内

	int centerX = pos.x + width / 2;
	int centerY = pos.y + height / 2;

	bool isInPlayerX = centerX >= player.x() && centerX <= player.x() + player.width;
	bool isInPlayerY = centerY >= player.y() && centerY <= player.y() + player.height;

	return isInPlayerX && isInPlayerY;
}



void Enemy::draw(int frameInterval) {

	//绘制阴影
	int x = pos.x + (width - shadowWidth) / 2;
	int y = pos.y + height - 35;
	putImageAlpha(x, y, &enemyShadow);

	//等于0时，指向原先的面部朝向
	if (isFacingLeft)
		animationEnemyLeft->play(pos.x, pos.y, frameInterval);
	else
		animationEnemyRight->play(pos.x, pos.y, frameInterval);
}

void putImageAlpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();

	/*
	AlphaBlend:	Windows GDI+ API，用于图像混合。
		GetImageHDC(nullptr), x, y, w, h:
			GetImageHDC(nullptr):获取屏幕
			x, y, w, h:	屏幕的位置,作为目标区域。(左上角坐标为x,y，宽为w，高为h)
		GetImageHDC(img), 0, 0, w, h:
			GetImageHDC(img):获取图像
			0, 0, w, h:	整个图像，作为源区域。

		{ AC_SRC_OVER,0,255, AC_SRC_ALPHA }: 将源图像以透明的方式覆盖到目标图像上，透明度由源图像的Alpha通道控制。
			AC_SRC_OVER:	源图像覆盖目标图像
			0,255:			参数，此处无作用
			AC_SRC_ALPHA:	指定源图像的Alpha通道覆盖
				图像的Alpha通道: 是图像的透明度通道，存储着每个像素的透明度信息
	*/
	AlphaBlend(GetImageHDC(nullptr), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255, AC_SRC_ALPHA });
}

void generateEnemy(std::vector<Enemy*>& enemys) {

	static const int interval = 25;
	static int timer = 0;

	if (timer % interval == 0) {
		auto enemy = new Enemy;
		enemys.push_back(enemy);
	}

	timer++;
	timer %= interval;
}

void updateBullets(std::vector<Bullet>& bullets, const Player& player) {
	static const double radialCoefficient = 0.0045;			//径系数
	static const double tangentCoefficient = 0.0055;		//切系数
	static const double Pi = 3.1415926;

	//弧度制
	double radianInterval = 2 * Pi / bullets.size();		//计算 子弹间隔

	//半径:		随时间和径系数变化
	double r = 100 + 25 * sin(GetTickCount() * radialCoefficient);

	for (int i = 0; i < bullets.size(); i++) {

		//角度:	随时间和切系数变化
		double radian = GetTickCount() * tangentCoefficient + radianInterval * i;

		bullets[i].pos.x = player.x() + player.width / 2 + int(r * cos(radian));
		bullets[i].pos.y = player.y() + player.height / 2 + int(r * sin(radian));
	}
}

void drawScore(const int& score) {
	static TCHAR text[64];

	_stprintf_s(text, _T("当前玩家得分: %d"), score);

	setbkmode(TRANSPARENT);			//将背景设为透明
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}

Animation::Animation(LPCTSTR path, int num, int frameInterval) : frameInterval(frameInterval) {
	TCHAR tPath[256];	// TCHAR: 更通用的字符

	for (int i = 0; i < num; i++) {
		/*
		将一个格式化的字符串 写入到 tPath 指向的字符数组中。
			格式 由 path 字符串模板决定，其中包含一些格式占位符（比如 %d、%s 等）。
			i 变量的值将用来替换 path 中对应的格式占位符。
		*/
		_stprintf_s(tPath, path, i);

		IMAGE* img = new IMAGE;

		loadimage(img, tPath);

		imgs.push_back(img);
	}
}

Animation::~Animation() {
	for (int i = 0; i < imgs.size(); i++)
		delete imgs[i];
}

void Animation::play(int x, int y, int playTime) {

	if (timer > frameInterval) {
		imgIndex++;
		imgIndex %= imgs.size();

		timer = 0;
	}

	putImageAlpha(x, y, imgs[imgIndex]);

	timer += playTime;
}


