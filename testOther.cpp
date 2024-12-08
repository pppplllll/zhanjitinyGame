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

	int frameInterval = 0;								//֡���ms: ��֡�������ʱ��

	int timer = 0;										//��ʱ��: ���Ŷ����Ѿ����ŵ�ʱ��

public:
	Animation(LPCTSTR path, int num, int frameInterval);//LPCSTR ��ͨ�õĳ��ַ�ָ��
	~Animation();
public:
	//����һ�Ŷ���
	void play(int x, int y, int playTime);				//playTime :���Ŷ������ŵ�ʱ��
};

class Player {
private:
	POINT playerPos{ 500,500 };							//���λ��

	const int playerSpeed = 6;							//�ƶ��ٶ�

	bool isLeft = false, isRight = false,				//�ƶ�����
		isUp = false, isDown = false;

	bool isFacingLeft = false;							//�沿����

	const int shadowWidth = 32;							//�����Ӱ�߶�

public:
	const int width = 80;								//��Ҹ߶�
	const int height = 80;

private:
	IMAGE playerShadow;
	Animation* animationPlayerLeft;						//��Ҷ���
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
		//�Ⱥ�ɫ���Բ
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));

		fillcircle(pos.x, pos.y, radius);
	}
};

class Enemy {
private:
	POINT pos{ 0,0 };									//����λ��

	const int width = 80;								//���˸߶�
	const int height = 80;
	const int shadowWidth = 48;							//������Ӱ�߶�

	const int enemySpeed = 2;							//�ƶ��ٶ�

	bool isLeft = false, isRight = false,				//�ƶ�����
		isUp = false, isDown = false;

	bool isFacingLeft = false;							//�沿����	

	bool isAlive = true;									//������

private:
	IMAGE enemyShadow;
	Animation* animationEnemyLeft;						//���˶���
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

void putImageAlpha(int x, int y, IMAGE* img);								//ͼ�����(͸����)
void generateEnemy(std::vector<Enemy* >& enemys);							//���ɵ���
void updateBullets(std::vector<Bullet>& bullets, const Player& player);		//�����ӵ�
void drawScore(const int& score);

//��ť��
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

//��������
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

//��ʼ��Ϸ��ť
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

//�˳���Ϸ��ť
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

	//���ص�ͼ
	IMAGE background;
	IMAGE img_menu;;
	

	//����mp3 ȡalias bgm
	mciSendString(_T("open mus/bgm.mp3 alias bgm"), nullptr, 0, nullptr);
	mciSendString(_T("open mus/hit.wav alias hit"), nullptr, 0, nullptr);
	//�ظ�����bgm ��0��ʼ
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

		//������
		peekmessage(&message);

		//��������
		if(is_game_started)
			player.processMessage(message);
		else
		{
			//btn_start_game.ProcessEvent(msg);
			btn_start_game.ProcessEvent(message);
			btn_quit_game.ProcessEvent(message);
		}

		if (is_game_started) {
			//�������
			player.move();

			//�����ӵ�
			updateBullets(bullets, player);

			generateEnemy(enemys);
			for (auto& enemy : enemys)
				enemy->move(player);

			//������ӵ���ײ���
			for (auto& enemy : enemys)
				for (const auto& bullet : bullets)
					if (enemy->checkBulletCollision(bullet)) {
						score++;
						enemy->hurt();
						mciSendString(_T("play hit from 0"), nullptr, 0, nullptr);
					}

			//����������ײ���
			for (auto& enemy : enemys) {
				if (enemy->checkPlayerCollision(player)) {
					TCHAR text[64];
					_stprintf_s(text, _T("���յ÷�: %d"), score);
					MessageBox(GetHWnd(), text, _T("��Ϸ����"), MB_OK);

					running = false;
					break;
				}
			}

			//�Ƴ�����ʧ�Ĺ���
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
		

		//��Ⱦ
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
		//120ˢ��
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

	//�ж��ƶ�����
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

//�����ƶ���Ϣ
void Player::move() {

	// x,y ���� ����
	int x = isRight - isLeft;
	int y = isDown - isUp;

	double modulus = sqrt(x * x + y * y);	//������ģ

	if (modulus) {

		double vectorX = x / modulus;
		double vectorY = y / modulus;

		playerPos.x += int(playerSpeed * vectorX);
		playerPos.y += int(playerSpeed * vectorY);
	}

	//У׼
	if (playerPos.x < 0)	playerPos.x = 0;
	if (playerPos.y < 0)	playerPos.y = 0;

	if (playerPos.x + width > windowWidth)	playerPos.x = windowWidth - width;
	if (playerPos.y + height > windowHeight)	playerPos.y = windowHeight - height;

	//�޸��沿����
		//����0ʱ��ָ��ԭ���沿����
	if (x > 0)
		isFacingLeft = false;
	else if (x < 0)
		isFacingLeft = true;

}

void Player::draw(int frameInterval) {

	//������Ӱ
	int xShadow = playerPos.x + (width - shadowWidth) / 2;
	int yShadow = playerPos.y + height - 8;
	putImageAlpha(xShadow, yShadow, &playerShadow);

	//���ƶ���
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

	//����������ƶ�
	int x = player.x() - pos.x;
	int y = player.y() - pos.y;

	double modulus = sqrt(x * x + y * y);	//������ģ

	if (modulus) {

		double vectorX = x / modulus;
		double vectorY = y / modulus;

		pos.x += int(enemySpeed * vectorX);
		pos.y += int(enemySpeed * vectorY);
	}

	//�޸��沿����
	if (x > 0)
		isFacingLeft = false;
	else if (x < 0)
		isFacingLeft = true;
}

bool Enemy::checkBulletCollision(const Bullet& bullet) const {

	//�ж��ӵ� �Ƿ��� ������
	bool isInX = bullet.pos.x >= pos.x && bullet.pos.x <= pos.x + width;

	bool isInY = bullet.pos.y >= pos.y && bullet.pos.y <= pos.y + height;

	return isInX && isInY;
}

bool Enemy::checkPlayerCollision(const Player& player) const {
	//�ж����ĵ� �Ƿ��� ��Ҿ�����

	int centerX = pos.x + width / 2;
	int centerY = pos.y + height / 2;

	bool isInPlayerX = centerX >= player.x() && centerX <= player.x() + player.width;
	bool isInPlayerY = centerY >= player.y() && centerY <= player.y() + player.height;

	return isInPlayerX && isInPlayerY;
}



void Enemy::draw(int frameInterval) {

	//������Ӱ
	int x = pos.x + (width - shadowWidth) / 2;
	int y = pos.y + height - 35;
	putImageAlpha(x, y, &enemyShadow);

	//����0ʱ��ָ��ԭ�ȵ��沿����
	if (isFacingLeft)
		animationEnemyLeft->play(pos.x, pos.y, frameInterval);
	else
		animationEnemyRight->play(pos.x, pos.y, frameInterval);
}

void putImageAlpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();

	/*
	AlphaBlend:	Windows GDI+ API������ͼ���ϡ�
		GetImageHDC(nullptr), x, y, w, h:
			GetImageHDC(nullptr):��ȡ��Ļ
			x, y, w, h:	��Ļ��λ��,��ΪĿ������(���Ͻ�����Ϊx,y����Ϊw����Ϊh)
		GetImageHDC(img), 0, 0, w, h:
			GetImageHDC(img):��ȡͼ��
			0, 0, w, h:	����ͼ����ΪԴ����

		{ AC_SRC_OVER,0,255, AC_SRC_ALPHA }: ��Դͼ����͸���ķ�ʽ���ǵ�Ŀ��ͼ���ϣ�͸������Դͼ���Alphaͨ�����ơ�
			AC_SRC_OVER:	Դͼ�񸲸�Ŀ��ͼ��
			0,255:			�������˴�������
			AC_SRC_ALPHA:	ָ��Դͼ���Alphaͨ������
				ͼ���Alphaͨ��: ��ͼ���͸����ͨ�����洢��ÿ�����ص�͸������Ϣ
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
	static const double radialCoefficient = 0.0045;			//��ϵ��
	static const double tangentCoefficient = 0.0055;		//��ϵ��
	static const double Pi = 3.1415926;

	//������
	double radianInterval = 2 * Pi / bullets.size();		//���� �ӵ����

	//�뾶:		��ʱ��;�ϵ���仯
	double r = 100 + 25 * sin(GetTickCount() * radialCoefficient);

	for (int i = 0; i < bullets.size(); i++) {

		//�Ƕ�:	��ʱ�����ϵ���仯
		double radian = GetTickCount() * tangentCoefficient + radianInterval * i;

		bullets[i].pos.x = player.x() + player.width / 2 + int(r * cos(radian));
		bullets[i].pos.y = player.y() + player.height / 2 + int(r * sin(radian));
	}
}

void drawScore(const int& score) {
	static TCHAR text[64];

	_stprintf_s(text, _T("��ǰ��ҵ÷�: %d"), score);

	setbkmode(TRANSPARENT);			//��������Ϊ͸��
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}

Animation::Animation(LPCTSTR path, int num, int frameInterval) : frameInterval(frameInterval) {
	TCHAR tPath[256];	// TCHAR: ��ͨ�õ��ַ�

	for (int i = 0; i < num; i++) {
		/*
		��һ����ʽ�����ַ��� д�뵽 tPath ָ����ַ������С�
			��ʽ �� path �ַ���ģ����������а���һЩ��ʽռλ�������� %d��%s �ȣ���
			i ������ֵ�������滻 path �ж�Ӧ�ĸ�ʽռλ����
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


