#include <iostream>
#include "winApiUsing.h"
#include <raylib.h>
#include <vector>
#include <sstream>
#include <unordered_set>

using namespace std;
atomic<int> screenRate = 0;

struct Vector3I {
	int x, y, z;
};

inline float getSizeOfText(const char* text, Rectangle rect, float spacing) {
	float base = 100.0f;
	Vector2 measure = MeasureTextEx(GetFontDefault(), text, base, spacing);
	float scale = fminf(rect.width / measure.x, rect.height / measure.y);
	return base * scale * 0.75f;
}

inline float getDistance(Vector2 a, Vector2 b) {
	return sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

inline Color lerp(Color start, Color end, float t) {
	auto getLerp = [&](unsigned char start, unsigned char end) -> unsigned char {
		float result = start + (end-start)*t;

		if (result < 0) result = 0;
		if (result > 255) result = 255;

		return static_cast<unsigned char>(result);
		};

	return {
		getLerp(start.r, end.r),
		getLerp(start.g, end.g),
		getLerp(start.b, end.b),
		getLerp(start.a, end.a),
	};
}

inline int lerp(int start, int end, float t) {
	return (start + (end - start) * t);
}

inline float lerp(float start, float end, float t) {
	return (start + (end - start) * t);
}

namespace Settings {
	namespace graphic {
		bool abilityEffects = true;
		bool drawGrid = true;
		bool renderEat = true;
		bool playersBorder = true;

		string settingsNames[] = { "Ability Effects", "Grid Render", "Squares Render", "Players Border" };
	}

	namespace game {
		Vector3I EatBallsQuantity = {5000, 1000, 15000};
		Vector3I BotsQuantity = {150, 50, 350};
		Vector3I MapSize = {10000, 5000, 15000};

		string settingsNames[] = {"Squares Quantity", "Bots Quantity", "Map Size"};
	}

	namespace binds {
		const int bindsQuantity = 15;
		KeyboardKey bindKeys[bindsQuantity] = { KEY_Q, KEY_E, KEY_W, KEY_R, KEY_T, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_TAB, KEY_ESCAPE, KEY_A, KEY_S, KEY_D, KEY_F };
		string bindKeysName[bindsQuantity] = { "Q", "E", "W", "R", "T", "Z", "X", "C", "V", "TAB", "ESC", "A", "S", "D", "F" };

		string bindsName[] = {"Ability", "Shot", "Pause"};
		
		KeyboardKey ability = KEY_E;
		KeyboardKey shot = KEY_C;
		KeyboardKey pause = KEY_ESCAPE;
	}
}

namespace Animate {

	class Animation {
		string _AnimationType;

		float _Time = 0.0f;
		float _TimePassed = 0.0f;

		int _StartValueInt;
		float _StartValueFloat;
		Color _StartValueColor;
		void* _EndValue = nullptr;

	public:
		void* _Object = nullptr;
		bool _Playing = false;

		void updateAnimation(float dt) {
			if (_Playing) {
				float lerpValue = _TimePassed / _Time;
				if (lerpValue > 1) lerpValue = 1;

				if (_AnimationType == "int") {
					*static_cast<int*>(_Object) = lerp(_StartValueInt, *static_cast<int*>(_EndValue), lerpValue);
				}
				else if (_AnimationType == "float") {
					*static_cast<float*>(_Object) = lerp(_StartValueFloat, *static_cast<float*>(_EndValue), lerpValue);
				}
				else if (_AnimationType == "Color") {
					*static_cast<Color*>(_Object) = lerp(_StartValueColor, *static_cast<Color*>(_EndValue), lerpValue);
				}

				_TimePassed += dt;

				if (_TimePassed > _Time) {
					_Playing = false;

					if (_AnimationType == "int") { *static_cast<int*>(_Object) = *static_cast<int*>(_EndValue); }
					if (_AnimationType == "float") { *static_cast<float*>(_Object) = *static_cast<float*>(_EndValue); }
					if (_AnimationType == "Color") { *static_cast<Color*>(_Object) = *static_cast<Color*>(_EndValue); }
				}
			}
		}

		void Stop() {
			_Playing = false;
		}

		Animation() = delete;
		Animation(int* objPtr, float Time, void* endValue) : _Time(Time), _Object(objPtr), _EndValue(endValue), _StartValueInt(*objPtr) {
			_AnimationType = "int";
			_Playing = true;
		};
		Animation(float* objPtr, float Time, void* endValue) : _Time(Time), _Object(objPtr), _EndValue(endValue), _StartValueFloat(*objPtr) {
			_AnimationType = "float";
			_Playing = true;
		};
		Animation(Color* objPtr, float Time, void* endValue) : _Time(Time), _Object(objPtr), _EndValue(endValue), _StartValueColor(*objPtr) {
			_AnimationType = "Color";
			_Playing = true;
		};

		~Animation() {}
	};

	vector<Animation*> ActiveAnimations;

	void onAnimCreate(void* objPtr) {
		for (Animation* anim : ActiveAnimations) {
			if (anim->_Object == objPtr) {
				anim->Stop();
			}
		}
	}

	bool animationWithPtr(void* objPtr) {
		for (Animation* anim : ActiveAnimations) {
			if (anim->_Object == objPtr) {
				return true;
			}
		}
		return false;
	}

	Animation* Create(int* obj, float Time, void* endValue, bool oneAnimOneObject) {
		if (animationWithPtr(obj) and oneAnimOneObject) {
			return nullptr;
		}
		onAnimCreate(obj);
		Animation* animationObject = new Animation(obj, Time, endValue);
		ActiveAnimations.push_back(animationObject);
		return animationObject;
	}
	Animation* Create(float* obj, float Time, void* endValue, bool oneAnimOneObject) {
		if (animationWithPtr(obj) and oneAnimOneObject) {
			return nullptr;
		}
		onAnimCreate(obj);
		Animation* animationObject = new Animation(obj, Time, endValue);
		ActiveAnimations.push_back(animationObject);
		return animationObject;
	}
	Animation* Create(Color* obj, float Time, void* endValue, bool oneAnimOneObject) {
		if (animationWithPtr(obj) and oneAnimOneObject) {
			return nullptr;
		}
		onAnimCreate(obj);
		Animation* animationObject = new Animation(obj, Time, endValue);
		ActiveAnimations.push_back(animationObject);
		return animationObject;
	}

	void updateAllAnimations() {
		float FrameDelta = GetFrameTime();

		for (auto animation = Animate::ActiveAnimations.begin(); animation != Animate::ActiveAnimations.end(); ) {
			Animate::Animation* animPtr = *animation;
			animPtr->updateAnimation(FrameDelta);

			if (animPtr->_Playing) {
				animation++;
			}
			else {
				delete(animPtr);
				animation = Animate::ActiveAnimations.erase(animation);
			}
		}
	}
}

inline void initRandom() {
	static bool inited = false;
	if (!inited) {
		srand(static_cast<unsigned int>(time(0)));
		inited = true;
	}
}

int random(int min, int max) {
	initRandom();
	return min + ((rand() << 15) | rand()) % (max - min + 1);
}

int BallsSpeedUp = 200;

class Ball {
public:
	Vector2 _Position = { 0.0f, 0.0f };
	float _Size = 0;
	string _Name = "";
	Color _Color;

	Ball() {
		_Color = { static_cast<unsigned char>(random(0, 255)), static_cast<unsigned char>(random(0, 255)), static_cast<unsigned char>(random(0, 255)), 255 };
	}

	Ball(Vector2 startPos) : Ball() { _Position = startPos; }
	Ball(Vector2 startPos, string Name) : Ball(startPos) { _Name = Name; }
	Ball(Vector2 startPos, string Name, float Size) : Ball(startPos, Name) { _Size = Size; }
	Ball(Vector2 startPos, string Name, float Size, Color c) : Ball(startPos, Name, Size) { _Color = c; }

	~Ball() {
		
	}
};

class BallCum : public Ball { // O-O Cu..Whaat..?
public:
	using Ball::Ball;

	Vector2 _MovingPos = { 0.0f, 0.0f };
	bool _Moving = false;
	int _speedPixelsPerSecond = 1000;
	float _timePassed = 0.0f;

	void moveTo(Vector2 worldPos) {
		if (worldPos.x < -Settings::game::MapSize.x / 2) { worldPos.x = -Settings::game::MapSize.x / 2; }
		if (worldPos.y < -Settings::game::MapSize.x / 2) { worldPos.y = -Settings::game::MapSize.x / 2; }
		if (worldPos.x > Settings::game::MapSize.x / 2) { worldPos.x = Settings::game::MapSize.x / 2; }
		if (worldPos.y > Settings::game::MapSize.x / 2) { worldPos.y = Settings::game::MapSize.x / 2; }
		_Moving = true;
		_MovingPos = worldPos;
	}

	void updateMoving(float dt) {
		_timePassed += dt;
		if (!_Moving) { return; }

		float distance = getDistance(_MovingPos, _Position);

		if ((int)distance == 0) {
			_Moving = false;
			return;
		}

		float t = (_speedPixelsPerSecond * dt) / distance;
		if (t > 1.0f) { t = 1.0f; _Moving = false; }
		_Position = { lerp(_Position.x, _MovingPos.x, t) , lerp(_Position.y, _MovingPos.y, t) };
	}
};

struct MatrixV {
	int rows, cols;

	vector<vector<Ball*>> data;

	MatrixV(int r, int c) : rows(r), cols(c), data(r* c) {}

	vector<Ball*>* operator()(int i, int j) {
		return &data[i * cols + j];
	}
};

enum class Abilities {
	SPEEDUP = 0,
	SHIELD = 1,
	INVISIBILITY = 2,
};
const int abilitiesQuantity = 3;

int endSpeedOfShot = 400;

class BallPlayer : public Ball {

	void updateAI(float dt, vector<BallPlayer*>* players) {
		static BallPlayer* lastTarget = nullptr;
		static float targetTime = 0.0f;
		static BallPlayer* targetExclude = nullptr;

		_actionRemaining -= dt;
		if (_actionRemaining > 0) return;

		_actionRemaining = _actionCooldown;

		BallPlayer* nearestBigger = nullptr;
		BallPlayer* nearestSmaller = nullptr;
		float BiggerDistance = 1e9;
		float SmallerDistance = 1e9;

		for (auto* p : *players) {
			if (p == this) continue;
			if (p->_Name == "Player") continue;

			float d = getDistance(_Position, p->_Position);

			if (p->_Size > _Size * 1.03f and d < BiggerDistance) {
				nearestBigger = p;
				BiggerDistance = d;
			}
			else if (p->_Size < _Size * 0.97f and d < SmallerDistance and p != targetExclude) {
				nearestSmaller = p;
				SmallerDistance = d;
			}
		}

		if (lastTarget != nullptr) {
			targetTime += dt;
			if (targetTime > 3.0f) {
				targetExclude = lastTarget;
				lastTarget = nullptr;
				targetTime = 0.0f;
			}
		}

		if (nearestBigger and BiggerDistance < 200 + nearestBigger->_Size) {
			Vector2 dir = { _Position.x - nearestBigger->_Position.x, _Position.y - nearestBigger->_Position.y };
			float len = sqrt(dir.x * dir.x + dir.y * dir.y);
			if (len > 0) { dir.x /= len; dir.y /= len; }

			dir.x += (rand() % 200 - 100) / 500.0f;
			dir.y += (rand() % 200 - 100) / 500.0f;

			if (_Size > 5.5 and getDistance(nearestBigger->_Position, _Position) < 20) speedUp();

			moveTo({ _Position.x + dir.x * 150.0f, _Position.y + dir.y * 150.0f });
			return;
		}

		if (nearestSmaller) {
			if (lastTarget != nearestSmaller) {
				lastTarget = nearestSmaller;
				targetTime = 0.0f;
			}

			if (lastTarget) {
				Vector2 target = lastTarget->_Position;
				target.x += (rand() % 200 - 100) / 5.0f;
				target.y += (rand() % 200 - 100) / 5.0f;

				float dist = getDistance(_Position, target);
				if (_Size > 5.5 and dist > 20) speedUp();

				moveTo(target);
				return;
			}
		}

		static Vector2 randomTarget = _Position;
		static float changeInterval = 2.0f;
		static float timeFromChange = 0.0f;

		timeFromChange += dt;
		if (timeFromChange >= changeInterval || !_Moving) {
			randomTarget = { _Position.x + (rand() % 400 - 200), _Position.y + (rand() % 400 - 200) };
			if (randomTarget.x < -Settings::game::MapSize.x / 2) randomTarget.x = -Settings::game::MapSize.x / 2;
			if (randomTarget.y < -Settings::game::MapSize.x / 2) randomTarget.y = -Settings::game::MapSize.x / 2;
			if (randomTarget.x > Settings::game::MapSize.x / 2) randomTarget.x = Settings::game::MapSize.x / 2;
			if (randomTarget.y > Settings::game::MapSize.x / 2) randomTarget.y = Settings::game::MapSize.x / 2;

			randomTarget.x += (rand() % 200 - 100);
			randomTarget.y += (rand() % 200 - 100);

			timeFromChange = 0.0f;
		}

		moveTo(randomTarget);
	}

public:
	using Ball::Ball;

	Abilities _Ability = Abilities::SPEEDUP;

	const float _actionCooldown = 0.3f;
	float _actionRemaining = 0.0f;

	bool _Player = false;

	BallCum* _babyPtr = nullptr;

	Vector2 _MovingPos = { 0.0f, 0.0f };
	bool _Moving = false;
	int _speedPixelsPerSecond = 100;
	float _speedUpRemaining = 0.0f;

	Vector2* _lastPositions = new Vector2[5];
	uint8_t _index = 0;
	float trailCooldown = 0.03f;

	void moveTo(Vector2 worldPos) {
		if (worldPos.x < -Settings::game::MapSize.x / 2) { worldPos.x = -Settings::game::MapSize.x / 2; }
		if (worldPos.y < -Settings::game::MapSize.x / 2) { worldPos.y = -Settings::game::MapSize.x / 2; }
		if (worldPos.x > Settings::game::MapSize.x / 2) { worldPos.x = Settings::game::MapSize.x / 2; }
		if (worldPos.y > Settings::game::MapSize.x / 2) { worldPos.y = Settings::game::MapSize.x / 2; }
		_Moving = true;
		_MovingPos = worldPos;
	}

	void cccShot() {
		if (!_Moving) return;
		if (_babyPtr != nullptr) return;

		_Size /= 2;
		_babyPtr = new BallCum;
		_babyPtr->_Color = _Color;
		_babyPtr->_Name = _Name;
		_babyPtr->_Size = _Size;
		_babyPtr->_speedPixelsPerSecond = 1000 * (_Size / 50.0f);

		float dx = _MovingPos.x - _Position.x;
		float dy = _MovingPos.y - _Position.y;
		float length = sqrt(dx * dx + dy * dy);
		if (length == 0) length = 1;

		float dirX = dx / length;
		float dirY = dy / length;

		float distance = 200.0f * (_Size / 50.0f);

		float targetX = _Position.x + dirX * distance;
		float targetY = _Position.y + dirY * distance;

		_babyPtr->_Position = { _Position.x, _Position.y };
		_babyPtr->moveTo({ targetX, targetY });

		Animate::Create(&_babyPtr->_speedPixelsPerSecond, 0.3, &endSpeedOfShot, false);
	}

	void updateMoving(float dt, vector<BallPlayer*>* players) {

		if (!this->_Player) {
			updateAI(dt, players);
		}

		trailCooldown -= dt;
		if (trailCooldown <= 0) {
			trailCooldown = 0.03f;
			_lastPositions[_index] = _Position;
			_index = (_index + 1) % 5;
		}

		if (_speedUpRemaining > 0) {
			_speedUpRemaining -= dt;
			if (_speedUpRemaining <= 0) {
				_speedUpRemaining = 0;
				_speedPixelsPerSecond = 100;
			}
		}

		if (!_Moving) { return; }

		float distance = getDistance(_MovingPos, _Position);

		if ((int)distance == 0) {
			_Moving = false;
			return;
		}

		float speedMultiplierBySize = 1;

		float defaultSize = 50.0f;
		float maxSpeedMultiplier = 1.2f;
		float minSpeedMultiplier = 0.7f;

		speedMultiplierBySize = 1.0f - ((_Size - defaultSize) / 1000.0f);

		if (speedMultiplierBySize < minSpeedMultiplier) speedMultiplierBySize = minSpeedMultiplier;
		if (speedMultiplierBySize > maxSpeedMultiplier) speedMultiplierBySize = maxSpeedMultiplier;


		float t = (_speedPixelsPerSecond * speedMultiplierBySize * dt) / distance;
		if (t > 1.0f) { t = 1.0f; _Moving = false; }
		_Position = { lerp(_Position.x, _MovingPos.x, t) , lerp(_Position.y, _MovingPos.y, t) };
	}

	void speedUp() {
		if (_Ability != Abilities::SPEEDUP) return;

		if ((_speedPixelsPerSecond == 100 and _speedUpRemaining == 0) and _Size > 5.5) {
			Animate::Create(&_speedPixelsPerSecond, 0.3, &BallsSpeedUp, false);
			_speedUpRemaining = 3;
			float SizeCost = 0.5 * _Size / 25;
			if (SizeCost > 5) {
				SizeCost = 5;
			}
			_Size -= SizeCost;
		} else if ((_speedPixelsPerSecond == 100 or _speedUpRemaining < 0.3f) and _Size > 5.5) {
			_speedUpRemaining = 3;
			float SizeCost = 0.5 * _Size / 25;
			if (SizeCost > 5) {
				SizeCost = 5;
			}
			_Size -= SizeCost;
		}
	}

	void abilityUse() {
		if (_Ability == Abilities::SPEEDUP) {
			speedUp();
		}
	}

	~BallPlayer() { delete[](_lastPositions); if (_babyPtr != nullptr) delete(_babyPtr); }
};

vector<BallPlayer*> Players;
vector<Ball*> EatBalls;
vector<BallCum*>Cums; // O-O

MatrixV gridOfBalls(40, 40);

int winWidth;
int winHeight;

Vector2 positionOnMapFromCamera(Vector2 MousePosition, Vector2 CameraPosition, float CameraZoom) {
	Vector2 screenCenter = { winWidth * 0.5f, winHeight * 0.5f };
	Vector2 delta = { MousePosition.x - screenCenter.x, MousePosition.y - screenCenter.y };
	Vector2 worldPos = { CameraPosition.x + delta.x / CameraZoom, CameraPosition.y + delta.y / CameraZoom };
	return worldPos;
}

Vector2 positionOnCamFromMap(Vector2 WorldPosition, Vector2 CameraPosition, float CameraZoom) {
	Vector2 screenCenter = { winWidth * 0.5f, winHeight * 0.5f };
	Vector2 delta = { WorldPosition.x - CameraPosition.x, WorldPosition.y - CameraPosition.y };
	Vector2 screenPos = {screenCenter.x + delta.x * CameraZoom, screenCenter.y + delta.y * CameraZoom};
	return screenPos;
}

namespace playerManage { // only for local player
	BallPlayer* PlayerPtr = nullptr;
	Vector2 CameraPosition = { 0,0 };
	float CameraZoom = 1.0f;
	float CameraZoomChange = 1.0f;

	RenderTexture2D playerImage;

	Abilities Ability = Abilities::SPEEDUP;
	Color color = { 0,0,0,255 };

}

int transparencyOfWayLine = 30;
int endOfTransparencyMax = 150;
int endOfTransparencyMin = 30;

int lastwinWidth = 0;
int lastwinHeight = 0;

float cooldownBeforeBotSpawn = 0.1;
float botSpawnTime = 0;

float cooldownBeforeEatBalls = 0.1;
float lastEatBalls = 0;

int countEatBallsToAdd = 0;
float lastCameraZoomChange = 1.0f;

enum class gameState {
	MAIN = 0,
	SHOP = 1,
	SETTINGS = 2,
	GAMEPLAY = 3,
	GAMEPLAY_PAUSED = 4,
};

gameState stateOfGame = gameState::MAIN;

void gameplayFrame() {
	ClearBackground(DARKGRAY);

	// Background Lines
	if (Settings::graphic::drawGrid) {
		static RenderTexture2D gridTexture = LoadRenderTexture(100, 100);
		static bool TextureCreated = false;

		if (!TextureCreated) {
			TextureCreated = true;

			BeginTextureMode(gridTexture);

			for (int i = 0; i < 150; i++) {
				DrawLineEx(positionOnCamFromMap({ (float)(50 * i) - static_cast<float>(Settings::game::MapSize.x / 2), -static_cast<float>(Settings::game::MapSize.x / 2) }, playerManage::CameraPosition, playerManage::CameraZoom), positionOnCamFromMap({ (float)(50 * i) - static_cast<float>(Settings::game::MapSize.x / 2), static_cast<float>(Settings::game::MapSize.x / 2) }, playerManage::CameraPosition, playerManage::CameraZoom), 5, { 75,75,75,255 });
				DrawLineEx(positionOnCamFromMap({ -static_cast<float>(Settings::game::MapSize.x / 2), (float)(50 * i) - static_cast<float>(Settings::game::MapSize.x / 2) }, playerManage::CameraPosition, playerManage::CameraZoom), positionOnCamFromMap({ static_cast<float>(Settings::game::MapSize.x / 2), (float)(50 * i) - static_cast<float>(Settings::game::MapSize.x / 2) }, playerManage::CameraPosition, playerManage::CameraZoom), 5, { 75,75,75,255 });
			}

			EndTextureMode();
		}

		DrawTexturePro(gridTexture.texture, { 0, 0, (float)Settings::game::MapSize.x, (float)-Settings::game::MapSize.x }, { -playerManage::CameraPosition.x * playerManage::CameraZoom + (winWidth / 2 - Settings::game::MapSize.x / 2 * playerManage::CameraZoom), -playerManage::CameraPosition.y * playerManage::CameraZoom + (winHeight / 2 - Settings::game::MapSize.x / 2 * playerManage::CameraZoom), Settings::game::MapSize.x * playerManage::CameraZoom, Settings::game::MapSize.x * playerManage::CameraZoom }, { 0, 0 }, 0, WHITE);
	}

	// WayLine
	if (playerManage::PlayerPtr->_Moving) {
		Vector2 start = positionOnCamFromMap(playerManage::PlayerPtr->_Position, playerManage::CameraPosition, playerManage::CameraZoom);
		Vector2 end = positionOnCamFromMap(playerManage::PlayerPtr->_MovingPos, playerManage::CameraPosition, playerManage::CameraZoom);

		DrawLineEx(start, end, playerManage::PlayerPtr->_Size * 0.15f * playerManage::CameraZoom, { 61, 214, 219, static_cast<unsigned char>(transparencyOfWayLine) });
	}

	// Draw Balls for eat
	static RenderTexture2D target;

	if (lastwinWidth != winWidth or lastwinHeight != winHeight) {
		UnloadRenderTexture(target);
		target = LoadRenderTexture(winWidth, winHeight);
	}

	lastwinWidth = winWidth;
	lastwinHeight = winHeight;

	BeginTextureMode(target);
	ClearBackground(BLANK);

	for (Ball* ball : EatBalls) {
		Vector2 camPos = positionOnCamFromMap(ball->_Position, playerManage::CameraPosition, playerManage::CameraZoom);
		float size = ball->_Size * playerManage::CameraZoom;
		if (camPos.x + size < 0 or camPos.x - size > winWidth or camPos.y + size < 0 or camPos.y - size > winHeight)
			continue;
		if (Settings::graphic::renderEat) {
			DrawRectangleV({ camPos.x - size, camPos.y - size }, { size * 2, size * 2 }, Color{ ball->_Color.r, ball->_Color.g, ball->_Color.b, ball->_Color.a });
		} else {
			DrawPixelV({ camPos.x - size, camPos.y - size }, Color{ ball->_Color.r, ball->_Color.g, ball->_Color.b, ball->_Color.a });
		}
	}

	EndTextureMode();

	DrawTextureRec(target.texture, { 0, 0, (float)winWidth, -(float)winHeight }, { 0, 0 }, WHITE);
	
	// Draw player and bots trails
	if (Settings::graphic::abilityEffects) {
		for (BallPlayer* ball : Players) {
			if (ball->_speedUpRemaining == 0) { continue; }
			for (uint8_t i = 0; i < 5; i++) {
				Vector2 camPos = positionOnCamFromMap(ball->_lastPositions[i], playerManage::CameraPosition, playerManage::CameraZoom);
				float offset = ball->_Size + ball->_Size * 0.025;
				if (camPos.x + offset > 0 and camPos.x - offset < winWidth and camPos.y + offset > 0 and camPos.y - offset < winHeight) {
					DrawCircle(camPos.x, camPos.y, ball->_Size * playerManage::CameraZoom, { 255,255,255,10 });
				}
			}
		}
	}

	// Draw players babies
	for (BallCum* ball : Cums) {
		Vector2 camPos = positionOnCamFromMap(ball->_Position, playerManage::CameraPosition, playerManage::CameraZoom);
		float offset = ball->_Size + ball->_Size * 0.025;
		if (camPos.x + offset > 0 and camPos.x - offset < winWidth and camPos.y + offset > 0 and camPos.y - offset < winHeight) {
			DrawCircle(camPos.x, camPos.y, ball->_Size * playerManage::CameraZoom, ball->_Color);
		}
	}

	// Draw player and bots
	for (size_t i = 0; i < Players.size(); ++i) {
		for (size_t j = 0; j + 1 < Players.size() - i; ++j) {
			if (Players[j]->_Size > Players[j + 1]->_Size) {
				BallPlayer* tmp = Players[j];
				Players[j] = Players[j + 1];
				Players[j + 1] = tmp;
			}
		}
	}

	for (BallPlayer* ball : Players) {
		Vector2 camPos = positionOnCamFromMap(ball->_Position, playerManage::CameraPosition, playerManage::CameraZoom);
		float offset = ball->_Size + ball->_Size * 0.025;
		if (camPos.x + offset > 0 and camPos.x - offset < winWidth and camPos.y + offset > 0 and camPos.y - offset < winHeight) {
			if (Settings::graphic::playersBorder) DrawCircle(camPos.x, camPos.y, (ball->_Size + ball->_Size * 0.025) * playerManage::CameraZoom, { 0,0,0,255 });
			DrawCircle(camPos.x, camPos.y, ball->_Size * playerManage::CameraZoom, ball->_Color);
		}
	}

	// Position text for player

	ostringstream positionText;
	positionText << "Position: " << static_cast<int>(playerManage::PlayerPtr->_Position.x) << " " << static_cast<int>(playerManage::PlayerPtr->_Position.y) << endl;
	string stringPosition = positionText.str();

	DrawRectangle((int)winWidth * 0.8, (int)winHeight * 0.95, (int)winWidth * 0.2, (int)winHeight * 0.05, { 220,220,220,255 });
	DrawText(stringPosition.c_str(), (int)winWidth * 0.81, (int)winHeight * 0.96, (int)winHeight * 0.035, { 0,0,0,255 });


	// Get Fps
	ostringstream fps;
	fps << GetFPS() << " FPS" << endl;
	string fpsStr = fps.str();

	DrawRectangle((int)winWidth * 0.87, (int)winHeight * 0, (int)winWidth * 0.13, (int)winHeight * 0.05, { 220,220,220,255 });
	DrawText(fpsStr.c_str(), (int)winWidth * 0.88, (int)winHeight * 0.01, (int)winHeight * 0.035, { 0,0,0,255 });
}

void clearAll() {
	playerManage::PlayerPtr = nullptr;
	for (auto b : Players) delete b; Players.clear();
	for (auto b : EatBalls) delete b; EatBalls.clear();
	for (auto b : Cums) delete b; Cums.clear();

	for (int i = 0; i < 40; i++) {
		for (int j = 0; j < 40; j++) {
			gridOfBalls(i, j)->clear();
		}
	}

	playerManage::CameraZoom = 1;
	playerManage::CameraPosition = { 0,0 };
	playerManage::CameraZoomChange = 1;
}

float timeFromStartGame = 0.0f;

void startGame() {
	timeFromStartGame = 0.0f;
	stateOfGame = gameState::GAMEPLAY;
	clearAll();

	playerManage::PlayerPtr = new BallPlayer({ 300, 300 }, "Player", 25, playerManage::color);
	playerManage::PlayerPtr->_Player = true;
	playerManage::PlayerPtr->_MovingPos = { 300, 300 };
	playerManage::PlayerPtr->_Moving = false;
	playerManage::CameraZoom = 1.0f;
	playerManage::CameraPosition = playerManage::PlayerPtr->_Position;
	Players.push_back(playerManage::PlayerPtr);

	for (int i = 0; i < Settings::game::EatBallsQuantity.x; i++) {
		Vector2 pos = { (float)random(0, static_cast<float>(Settings::game::MapSize.x)) - static_cast<float>(Settings::game::MapSize.x / 2), (float)random(0, static_cast<float>(Settings::game::MapSize.x)) - static_cast<float>(Settings::game::MapSize.x / 2) };

		Ball* a = new Ball(pos, "", 5, {
			(unsigned char)random(0, 255),
			(unsigned char)random(0, 255),
			(unsigned char)random(0, 255),
			255
			});
		EatBalls.push_back(a);

		gridOfBalls((int)(pos.x / (Settings::game::MapSize.x / 39) + 20), (int)(pos.y / (Settings::game::MapSize.x / 39) + 20))->push_back(a);
	}
}

float timeFromStartPause = 1.0f;
int transparencyPauseBG = 0;
int endValPauseBG = 100;
int startValPauseBG = 0;

void pauseFrame(float dt) {
	Vector2 mousePosition = GetMousePosition();

	// Dark background
	DrawRectangle(0, 0, winWidth, winHeight, { 0,0,0,(unsigned char)transparencyPauseBG });
	
	// Buttons
	int sizeX = winWidth * 0.2;
	int sizeY = winHeight * 0.07;

	int endX = winWidth * 0.05;
	int startY = winHeight * 0.3;

	float lerpX = timeFromStartPause * 3; if (lerpX > 1) lerpX = 1;

	if (stateOfGame == gameState::GAMEPLAY) lerpX = 1;

	Vector2 buttonPoses[2] = {
		{ (sizeX + endX) / lerpX - sizeX, startY + sizeY * 1.1 * 0 },
		{ (sizeX + endX) / lerpX - sizeX, startY + sizeY * 1.1 * 2 },
	};

	string buttonTexts[2]{"Main Menu", "Restart"};

	for (int i = 0; i < 2; i++) {
		unsigned char clr = 224;

		if (mousePosition.x > buttonPoses[i].x and mousePosition.x < buttonPoses[i].x + sizeX and mousePosition.y > buttonPoses[i].y and mousePosition.y < buttonPoses[i].y + sizeY) {
			clr = 128;
		}

		DrawRectangle(buttonPoses[i].x, buttonPoses[i].y, sizeX, sizeY, { 32,32,32,(unsigned char)(255 * ((float)transparencyPauseBG / 100)) });
		DrawText(buttonTexts[i].c_str(), buttonPoses[i].x + winWidth * 0.01, buttonPoses[i].y + winHeight * 0.01, getSizeOfText(buttonTexts[i].c_str(), { buttonPoses[i].x, buttonPoses[i].y, (float)sizeX, (float)sizeY }, 0), { clr,clr,clr,(unsigned char)(255 * ((float)transparencyPauseBG / 100)) });
	}

	DrawText("Pause", winWidth * 0.03, winHeight * 0.02, getSizeOfText("Pause", { winWidth * 0.03f, winHeight * 0.02f, winWidth*0.3f, winHeight*0.2f}, 0), {255,255,255,(unsigned char)(255 * ((float)transparencyPauseBG / 100))});

	// Button click
	if (stateOfGame == gameState::GAMEPLAY_PAUSED and timeFromStartPause > 0.15f) {
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			for (int i = 0; i < 2; i++) {
				if (mousePosition.x > buttonPoses[i].x and mousePosition.x < buttonPoses[i].x + sizeX and mousePosition.y > buttonPoses[i].y and mousePosition.y < buttonPoses[i].y + sizeY) {
					if (buttonTexts[i] == "Main Menu") {
						stateOfGame = gameState::MAIN;
						clearAll();
					}
					else if (buttonTexts[i] == "Restart") {
						startGame();
					}

					break;
				}
			}
		}
	}
}

float endOfZoom = playerManage::CameraZoomChange;

void gameplayUpdate(float dt) {
	timeFromStartGame += dt;

	if (IsKeyPressed(Settings::binds::pause)) {
		if (timeFromStartPause >= 1.0f) {
			timeFromStartPause = 0.0f;
			if (stateOfGame == gameState::GAMEPLAY_PAUSED) stateOfGame = gameState::GAMEPLAY; else stateOfGame = gameState::GAMEPLAY_PAUSED;

			if (gameState::GAMEPLAY_PAUSED == stateOfGame) {
				Animate::Create(&transparencyPauseBG, 0.15f, &endValPauseBG, false);
			} else if (gameState::GAMEPLAY == stateOfGame) {
				Animate::Create(&transparencyPauseBG, 0.15f, &startValPauseBG, false);
			}
		}
	}

	timeFromStartPause += dt;

	if (stateOfGame == gameState::GAMEPLAY_PAUSED) {
		gameplayFrame();
		Animate::updateAllAnimations();
		pauseFrame(dt);
		return;
	}
		
	for (BallPlayer* b : Players) {
		b->updateMoving(dt, &Players);
	}

	for (BallCum* b : Cums) {
		b->updateMoving(dt);
	}

	// spawn bot
	botSpawnTime += dt;
	if (botSpawnTime > cooldownBeforeBotSpawn) {
		botSpawnTime = 0;
		if (Players.size() < Settings::game::BotsQuantity.x) {
			ostringstream n;
			n << "BOT_" << random(0, 2147483647);
			string name = n.str();

			BallPlayer* bot = new BallPlayer({ (float)random(0, static_cast<float>(Settings::game::MapSize.x)) - static_cast<float>(Settings::game::MapSize.x / 2), (float)random(0, static_cast<float>(Settings::game::MapSize.x)) - static_cast<float>(Settings::game::MapSize.x / 2) }, name, random(15, 25));
			Players.push_back(bot);
		}
	}

	// create eat balls
	for (int i = 0; i < countEatBallsToAdd; i++) {
		Vector2 pos = { (float)random(0, static_cast<float>(Settings::game::MapSize.x)) - static_cast<float>(Settings::game::MapSize.x / 2), (float)random(0, static_cast<float>(Settings::game::MapSize.x)) - static_cast<float>(Settings::game::MapSize.x / 2) };

		Ball* a = new Ball(pos, "", 5, {
		(unsigned char)random(0, 255),
		(unsigned char)random(0, 255),
		(unsigned char)random(0, 255),
		255
			});
		EatBalls.push_back(a);

		gridOfBalls((int)(pos.x / (Settings::game::MapSize.x / 39) + 20), (int)(pos.y / (Settings::game::MapSize.x / 39) + 20))->push_back(a);
	}

	countEatBallsToAdd = 0;

	// moving player ball
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		if (timeFromStartGame > 0.1f) {
			Vector2 mousePos = GetMousePosition();
			playerManage::PlayerPtr->moveTo(positionOnMapFromCamera(mousePos, playerManage::CameraPosition, playerManage::CameraZoom));
		}
	}

	// use ability player
	if (IsKeyPressed(Settings::binds::ability)) {
		playerManage::PlayerPtr->abilityUse();
	}

	// create baby
	if (IsKeyPressed(Settings::binds::shot)) {
		if (playerManage::PlayerPtr->_Moving and playerManage::PlayerPtr->_babyPtr == nullptr and playerManage::PlayerPtr->_Size > 75) {
			playerManage::PlayerPtr->cccShot();
			Cums.push_back(playerManage::PlayerPtr->_babyPtr);
		}
	}

	// animation of way line
	if (transparencyOfWayLine == 30) {
		transparencyOfWayLine = 31;
		
		Animate::Create(&transparencyOfWayLine, 1.0f, &endOfTransparencyMax, false);
	}
	else if (transparencyOfWayLine == 150) {
		transparencyOfWayLine = 149;
		
		Animate::Create(&transparencyOfWayLine, 1.0f, &endOfTransparencyMin, false);
	}

	// change camera zoom
	float wheel = GetMouseWheelMove();
	if (wheel < 0) {
		if (playerManage::CameraZoomChange > 0.2) {
			playerManage::CameraZoomChange -= 0.05;
		}
	}
	else if (wheel > 0) {
		if (playerManage::CameraZoomChange < 3) {
			playerManage::CameraZoomChange += 0.05;
		}
	}

	if (lastCameraZoomChange != playerManage::CameraZoomChange) {
		endOfZoom = playerManage::CameraZoomChange;
		Animate::Create(&(playerManage::CameraZoom), 0.3, &endOfZoom, true);
		lastCameraZoomChange = playerManage::CameraZoomChange;
	}

	// eat object
	if (lastEatBalls >= cooldownBeforeEatBalls) {
		lastEatBalls = 0;

		vector<BallPlayer*> plrs;

		for (int i = 0; i < Players.size(); i++) {
			plrs.push_back(Players[i]);
		}

		unordered_set<Ball*> ballsToDelete;

		float cellSize = Settings::game::MapSize.x / 40.0f;

		for (BallPlayer* player : plrs) {
			int xIndex = (int)(player->_Position.x / cellSize + 20);
			int yIndex = (int)(player->_Position.y / cellSize + 20);
			int reach = (int)ceil(player->_Size / cellSize);

			for (int deltaX = -reach; deltaX <= reach; deltaX++) {
				for (int deltaY = -reach; deltaY <= reach; deltaY++) {
					int nearX = xIndex + deltaX;
					int nearY = yIndex + deltaY;

					if (nearX < 0 or nearX >= 40 or nearY < 0 or nearY >= 40) continue;

					vector<Ball*>* arr = gridOfBalls(nearX, nearY);
					for (Ball* bl : *arr) {
						if (getDistance(player->_Position, bl->_Position) <= player->_Size) {
							ballsToDelete.insert(bl);
						}
					}
				}
			}
		}

		for (Ball* bl : ballsToDelete) {
			for (BallPlayer* player : plrs) {
				if (getDistance(player->_Position, bl->_Position) <= player->_Size) {
					player->_Size += ((bl->_Size * bl->_Size * 3.1415926) / (player->_Size * player->_Size * 3.1415926) * bl->_Size) * 4;
				}
			}

			auto itEat = find(EatBalls.begin(), EatBalls.end(), bl);
			if (itEat != EatBalls.end()) EatBalls.erase(itEat);

			int xIndex = (int)(bl->_Position.x / cellSize + 20);
			int yIndex = (int)(bl->_Position.y / cellSize + 20);

			if (xIndex < 0) xIndex = 0; if (xIndex >= 40) xIndex = 39;
			if (yIndex < 0) yIndex = 0; if (yIndex >= 40) yIndex = 39;

			auto gridArr = gridOfBalls(xIndex, yIndex);
			auto Grid = find(gridArr->begin(), gridArr->end(), bl);
			if (Grid != gridArr->end()) gridArr->erase(Grid);

			countEatBallsToAdd += 1;
			delete bl;
		}
	}

	for (auto plBall = Players.begin(); plBall != Players.end(); ) {
		BallPlayer* pl = *plBall;
		bool eaten = false;
		for (BallCum* cm : Cums) {
			if (getDistance(cm->_Position, pl->_Position) <= cm->_Size and cm->_Size > pl->_Size and cm != pl->_babyPtr) {
				cm->_Size += ((pl->_Size * pl->_Size * 3.1415926) / (cm->_Size * cm->_Size * 3.1415926) * pl->_Size) * 0.5;

				if (pl == playerManage::PlayerPtr) {

					Vector2 newPos = { random(0, Settings::game::MapSize.x) - Settings::game::MapSize.x / 2, random(0, Settings::game::MapSize.x) - Settings::game::MapSize.x / 2 };

					playerManage::PlayerPtr->_Size = 30;
					playerManage::PlayerPtr->_Position = newPos;
					playerManage::PlayerPtr->_MovingPos = newPos;

					if (playerManage::PlayerPtr->_babyPtr != nullptr) {
						delete playerManage::PlayerPtr->_babyPtr;
					}

					playerManage::PlayerPtr->_Moving = false;
					playerManage::PlayerPtr->_speedPixelsPerSecond = 100;
					playerManage::PlayerPtr->_speedUpRemaining = 0.0f;
					playerManage::PlayerPtr->_Color = { static_cast<unsigned char>(random(0, 255)), static_cast<unsigned char>(random(0, 255)), static_cast<unsigned char>(random(0, 255)), 255 };

				}
				else {
					delete pl;
					plBall = Players.erase(plBall);
					eaten = true;
				}

				break;
			}
		}
		if (!eaten) {
			plBall++;
		}
	}

	lastEatBalls += dt;

	for (auto plBall = Players.begin(); plBall != Players.end(); ) {
		BallPlayer* bl = *plBall;
		bool eaten = false;
		for (BallPlayer* player : Players) {
			if (getDistance(player->_Position, bl->_Position) <= player->_Size and player->_Size > bl->_Size and player != bl) {
				player->_Size += ((bl->_Size * bl->_Size * 3.1415926) / (player->_Size * player->_Size * 3.1415926) * bl->_Size) * 0.5;

				if (bl == playerManage::PlayerPtr) {

					Vector2 newPos = { random(0, Settings::game::MapSize.x) - Settings::game::MapSize.x / 2, random(0, Settings::game::MapSize.x) - Settings::game::MapSize.x / 2 };

					playerManage::PlayerPtr->_Size = 30;
					playerManage::PlayerPtr->_Position = newPos;
					playerManage::PlayerPtr->_MovingPos = newPos;

					if (playerManage::PlayerPtr->_babyPtr != nullptr) {
						delete playerManage::PlayerPtr->_babyPtr;
					}

					playerManage::PlayerPtr->_Moving = false;
					playerManage::PlayerPtr->_speedPixelsPerSecond = 100;
					playerManage::PlayerPtr->_speedUpRemaining = 0.0f;
					playerManage::PlayerPtr->_Color = { static_cast<unsigned char>(random(0, 255)), static_cast<unsigned char>(random(0, 255)), static_cast<unsigned char>(random(0, 255)), 255 };

				}
				else {
					delete bl;
					plBall = Players.erase(plBall);
					eaten = true;
				}

				break;
			}
		}
		if (!eaten) {
			plBall++;
		}
	}

	for (auto baby = Cums.begin(); baby != Cums.end(); ) {
		BallCum* bl = *baby;
		bool eaten = false;

		if (bl->_timePassed < 0.3) {
			baby++;
			continue;
		}

		for (BallPlayer* player : Players) {
			if ((getDistance(player->_Position, bl->_Position) <= player->_Size and ((player->_Size > bl->_Size) or bl == player->_babyPtr))) {
				player->_Size += ((bl->_Size * bl->_Size * 3.1415926) / (player->_Size * player->_Size * 3.1415926) * bl->_Size) * 1;

				for (BallPlayer* pl : Players) {
					if (pl->_Name == bl->_Name) {
						pl->_babyPtr = nullptr;
						continue;
					}
				}

				delete bl;
				baby = Cums.erase(baby);
				eaten = true;

				break;
			}
		}

		if (!eaten) {
			baby++;
		}
	}

	for (auto baby = Cums.begin(); baby != Cums.end(); ) {
		BallCum* bl = *baby;
		bool eaten = false;

		if (bl->_timePassed < 0.2) {
			baby++;
			continue;
		}

		for (BallCum* anotherBl : Cums) {
			if ((getDistance(anotherBl->_Position, bl->_Position) <= anotherBl->_Size and anotherBl->_Size > bl->_Size) and anotherBl != bl) {
				anotherBl->_Size += ((bl->_Size * bl->_Size * 3.1415926) / (anotherBl->_Size * anotherBl->_Size * 3.1415926) * bl->_Size) * 1;

				for (BallPlayer* pl : Players) {
					if (pl->_Name == bl->_Name) {
						pl->_babyPtr = nullptr;
					}
				}

				delete bl;
				baby = Cums.erase(baby);
				eaten = true;

				break;
			}
		}
		if (!eaten) {
			baby++;
		}
	}

	playerManage::CameraPosition = playerManage::PlayerPtr->_Position;

	Animate::updateAllAnimations();
	ClearBackground(DARKGRAY);

	gameplayFrame();

	if (stateOfGame == gameState::GAMEPLAY and timeFromStartPause < 0.1) {
		pauseFrame(dt);
	}
}

int lst = winWidth + winHeight;

RenderTexture2D grid;
bool TextureCreated = false;

void mainUpdate(float dt) {
	ClearBackground(DARKGRAY);

	Vector2 mousePosition = GetMousePosition();

	// background grid
	if (lst != winWidth + winHeight) TextureCreated = false;
	lst = winWidth + winHeight;

	if (!TextureCreated) {
		TextureCreated = true;

		grid = LoadRenderTexture(winWidth*1.2, winHeight*1.2);

		BeginTextureMode(grid);

		for (int i = 0; i < 100; i++) {
			DrawLineEx({(float)i * 75, -winHeight * 0.2f }, { (float)i * 75 , (float)winHeight * 1.2f }, 5, {75,75,75,255});
			DrawLineEx({-winWidth*0.2f, (float)i * 75 }, { (float)winWidth * 1.2f , (float)i * 75 }, 5, {75,75,75,255});
		}

		EndTextureMode();
	}

	DrawTexture(grid.texture, ((mousePosition.x / winWidth) - 0.5) * winWidth * 0.1 - winWidth * 0.1, ((mousePosition.y / winHeight) - 0.5) * winHeight * 0.1 - winHeight * 0.1, WHITE);
	DrawRectangle(0, 0, winWidth, winHeight, { 61,153,215,30 });

	// Name
	float startHeight = winHeight * 0.00f;
	float startWidth = winWidth * 0.45f;
	DrawRectangle(startWidth, startHeight, winWidth * 0.1f, winHeight * 0.07f, { 102, 102, 255, 255 });

	DrawTriangle(
		{ startWidth, startHeight },
		{ startWidth - winWidth * 0.05f, startHeight },
		{ startWidth,  startHeight + winHeight * 0.07f },
		{ 102, 102, 255, 255 }
	);

	DrawTriangle(
		{ startWidth + winWidth * 0.1f, startHeight },
		{ startWidth + winWidth * 0.1f, startHeight + winHeight * 0.07f },
		{ startWidth + winWidth * 0.1f + winWidth * 0.05f, startHeight },
		{ 102, 102, 255, 255 }
	);
	                                                                                                                                                                                           
	DrawText("Agar.io", winWidth * 0.45, startHeight, getSizeOfText("Agar.io", { winWidth * 0.45f, startHeight, winWidth * 0.1f, winHeight * 0.07f }, 0), { 204, 204, 255, 255 });

	// buttons
	Vector2 buttonSize = { winWidth * 0.2, winHeight * 0.1 };

	Vector2 buttons[3] = {
		{winWidth * 0.4, winHeight * (0.2 + (0.13 * 0))},
		{winWidth * 0.4, winHeight * (0.2 + (0.13 * 1))},
		{winWidth * 0.4, winHeight * (0.2 + (0.13 * 2))}
	};

	string buttonsText[3]{
		"Play", "Character", "Settings"
	};

	int _i = 0;
	for (Vector2 buttonPos : buttons) {
		string text = buttonsText[_i++];
		Color endColor = { 0, 51, 102, 255 };

		if (mousePosition.x > buttonPos.x and mousePosition.x < buttonPos.x + buttonSize.x and mousePosition.y>buttonPos.y and mousePosition.y < buttonPos.y + buttonSize.y) endColor = { 0, 102, 102, 255 };

		DrawRectangle(buttonPos.x - winWidth / 300, buttonPos.y - winHeight / 300, buttonSize.x + winWidth / 150, buttonSize.y + winHeight / 150, endColor);
		DrawRectangle(buttonPos.x, buttonPos.y, buttonSize.x, buttonSize.y, { 204, 229, 255, 255 });
		DrawText(text.c_str(), buttonPos.x + winWidth * 0.01f, buttonPos.y + winHeight * 0.01f, getSizeOfText(text.c_str(),
			{ buttonPos.x, buttonPos.y, buttonSize.x - winWidth * 0.01f, buttonSize.y - winHeight * 0.01f }, 0.0f), endColor);
	}

	int _j = 0;
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		for (Vector2 buttonPos : buttons) {
			if (mousePosition.x > buttonPos.x and mousePosition.x < buttonPos.x + buttonSize.x and mousePosition.y>buttonPos.y and mousePosition.y < buttonPos.y + buttonSize.y) {
				if (buttonsText[_j] == "Play") startGame();
				if (buttonsText[_j] == "Character") stateOfGame = gameState::SHOP;
				if (buttonsText[_j] == "Settings") stateOfGame = gameState::SETTINGS;
				break;
			};
			_j++;
		}
	}
}

Texture2D abilityTextures[abilitiesQuantity];
string abilityTexturesWays[abilitiesQuantity] = { "AbilityTextures/SpeedUpTexture.png", "AbilityTextures/ShieldTexture.png" , "AbilityTextures/InvisibilityTexture.png" };
bool loaded = false;

void characterUpdate(float dt) {
	ClearBackground(DARKGRAY);

	Vector2 mousePosition = GetMousePosition();

	// background grid
	if (lst != winWidth + winHeight) TextureCreated = false;
	lst = winWidth + winHeight;

	if (!TextureCreated) {
		TextureCreated = true;

		grid = LoadRenderTexture(winWidth * 1.2, winHeight * 1.2);

		BeginTextureMode(grid);

		for (int i = 0; i < 100; i++) {
			DrawLineEx({ (float)i * 75, -winHeight * 0.2f }, { (float)i * 75 , (float)winHeight * 1.2f }, 5, { 75,75,75,255 });
			DrawLineEx({ -winWidth * 0.2f, (float)i * 75 }, { (float)winWidth * 1.2f , (float)i * 75 }, 5, { 75,75,75,255 });
		}

		EndTextureMode();
	}

	DrawTexture(grid.texture, ((mousePosition.x / winWidth) - 0.5) * winWidth * 0.1 - winWidth * 0.1, ((mousePosition.y / winHeight) - 0.5) * winHeight * 0.1 - winHeight * 0.1, WHITE);
	DrawRectangle(0, 0, winWidth, winHeight, { 255,255,153,30 });

	//name
	float startHeight = winHeight * 0.00f;
	float startWidth = winWidth * 0.425f;
	float sizeX = winWidth * 0.15f;
	DrawRectangle(startWidth, startHeight, sizeX, winHeight * 0.07f, { 196, 183, 94, 255 });

	DrawTriangle(
		{ startWidth, startHeight },
		{ startWidth - sizeX/2, startHeight },
		{ startWidth,  startHeight + winHeight * 0.07f },
		{ 196, 183, 94, 255 }
	);

	DrawTriangle(
		{ startWidth + sizeX, startHeight },
		{ startWidth + sizeX, startHeight + winHeight * 0.07f },
		{ startWidth + sizeX + sizeX/2, startHeight },
		{ 196, 183, 94, 255 }
	);

	DrawText("Character", startWidth, startHeight, getSizeOfText("Character", { startWidth, startHeight, sizeX, winHeight * 0.07f }, 0), { 255, 251, 215, 255 });

	// back to menu
	DrawRectangle(0, winHeight*0.93, winWidth * 0.1, winHeight * 0.07, { 239, 230, 174, 255 });
	DrawText("Menu", winWidth * 0.01, winHeight * 0.94, getSizeOfText("Menu", { 0, winHeight * 0.93f, winWidth * 0.1f, winHeight * 0.07f }, 0), { 127, 115, 37, 255 });

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		if (mousePosition.x > 0 and mousePosition.x < 0 + winWidth * 0.1 and mousePosition.y>winHeight * 0.93 and mousePosition.y < winHeight * 0.93 + winHeight * 0.07) {
			stateOfGame = gameState::MAIN;
		}
	}

	// abilities choice
	Vector2 buttonSize1 = { winWidth * 0.1f, winWidth * 0.1f };
	Vector2 buttonPositions1[abilitiesQuantity] = {
		{winWidth * 0.8f, winHeight * 0.2f + buttonSize1.x * 1.1f * 0},
		{winWidth * 0.8f, winHeight * 0.2f + buttonSize1.x * 1.1f * 1},
		{winWidth * 0.8f, winHeight * 0.2f + buttonSize1.x * 1.1f * 2},
	};

	string names[abilitiesQuantity] = { "Speed up", "Shield", "Invisibility" };

	static int lastW = winWidth;
	static int lastH = winHeight;

	if (lastW != winWidth or lastH != winHeight) loaded = false;
	lastW = winWidth; lastH = winHeight;

	if (!loaded) {
		for (int i = 0; i < 3; i++) {
			abilityTextures[i] = LoadTexture(abilityTexturesWays[i].c_str());
		}

		loaded = true;
	}

	for (int i = 0; i < 3; i++) {
		Color clr = { 0,0,0,255 };
		if (i == static_cast<int>(playerManage::Ability)) clr = { 204,102,0,255 };

		DrawRectangle(buttonPositions1[i].x-winWidth*0.002, buttonPositions1[i].y - winWidth * 0.002, buttonSize1.x + winWidth * 0.006, buttonSize1.y + winWidth * 0.006, clr);
		DrawRectangle(buttonPositions1[i].x, buttonPositions1[i].y, buttonSize1.x, buttonSize1.y, { 255,204,153,255 });

		DrawTexturePro(abilityTextures[i], {0, 0, (float)abilityTextures[i].width, (float)abilityTextures[i].height}, { buttonPositions1[i].x + buttonSize1.x * 0.075f, buttonPositions1[i].y , buttonSize1.x*0.85f, buttonSize1.y*0.85f}, {0,0}, 0.0f, WHITE);
		
		DrawText(names[i].c_str(), buttonPositions1[i].x + buttonSize1.x*0.1, buttonPositions1[i].y + buttonSize1.y * 0.84, getSizeOfText(names[i].c_str(), {buttonPositions1[i].x , buttonPositions1[i].y + buttonSize1.y * 0.84f, buttonSize1.x, buttonSize1.y * 0.2f}, 0), clr);
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		for (int i = 0; i < 3; i++) {
			if (mousePosition.x > buttonPositions1[i].x and mousePosition.x < buttonPositions1[i].x + buttonSize1.x and mousePosition.y > buttonPositions1[i].y and mousePosition.y < buttonPositions1[i].x + buttonSize1.y) {
				playerManage::Ability = static_cast<Abilities>(i);
			}
		}
	}

	// draw character and his settings

	static int startValue = winHeight * 0.49;
	static int endValue = winHeight * 0.505;
	static int characterHeight = startValue;

	if (characterHeight == startValue) {
		characterHeight = startValue + 1;
		Animate::Create(&characterHeight, 1.5f, &endValue, false);
	}
	else if (characterHeight == endValue) {
		characterHeight = endValue - 1;
		Animate::Create(&characterHeight, 1.1f, &startValue, false);
	}

	static float sizeMultiplier = 0.9;
	static float minMultiplier = 0.9;
	static float maxMultiplier = 1.1;

	if (sizeMultiplier == minMultiplier) {
		if (getDistance({ winWidth * 0.2f, (float)characterHeight }, mousePosition) <= winWidth * 0.1 * sizeMultiplier) {
			sizeMultiplier = minMultiplier + 0.001f;
			Animate::Create(&sizeMultiplier, 0.2f, &maxMultiplier, true);
		}
	} else {
		if (getDistance({ winWidth * 0.2f, (float)characterHeight }, mousePosition) > winWidth * 0.1 * sizeMultiplier) {
			sizeMultiplier -= 0.001f;
			Animate::Create(&sizeMultiplier, 0.1f, &minMultiplier, true);
		}
	}

	DrawCircle(winWidth * 0.2, characterHeight, (winWidth * 0.1 + winWidth * 0.004) * sizeMultiplier, { 0,0,0,255 });
	DrawCircle(winWidth * 0.2, characterHeight, winWidth * 0.1 * sizeMultiplier, playerManage::color);

	// color of ball
	static unsigned char Colors[3] = {255, 255, 255};

	int CStartX = winWidth * 0.12;
	int CStartY = winHeight * 0.73;
	
	Vector2 CSize = {winWidth * 0.15, winHeight * 0.02};

	DrawRectangleRounded({ CStartX - winWidth * 0.01f, CStartY - winHeight * 0.01f, CSize.x * 1.14f, CSize.y * 3 * 1.41f }, 0.3, 5, { 40, 40, 40, 255 });
	DrawRectangleRoundedLinesEx({ CStartX - winWidth * 0.01f, CStartY - winHeight * 0.01f, CSize.x * 1.14f, CSize.y * 3 * 1.41f }, 0.3, 5, 4, { 20,20,20, 255 });

	static int downed = -1;

	for (int i = 0; i < 3; i++) {
		unsigned char _clr[4] = {0,0,0,255};
		_clr[i] = 255;
		DrawRectangleGradientH(CStartX, CStartY + CSize.y * 1.15 * i, CSize.x, CSize.y, {0,0,0,255}, { _clr[0], _clr[1], _clr[2], _clr[3] });

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) and (mousePosition.x >= CStartX and mousePosition.x <= CStartX + CSize.x and mousePosition.y >= CStartY + CSize.y * 1.15 * i and mousePosition.y <= CStartY + CSize.y * 1.15 * i + CSize.y)) {
			downed = i;
		}

		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
			downed = -1;
		}

		if (downed == i) {
			float z = ((mousePosition.x - CStartX) / CSize.x); if (z > 1) z = 1; if (z < 0) z = 0;
			Colors[i] = 255 * z;
		}

		DrawRectangle(CStartX + (((float)Colors[i] / 255) * CSize.x), CStartY + CSize.y * 1.15 * i - winHeight * 0.001, winWidth * 0.002, CSize.y + winHeight * 0.002, { 255,255,255,255 });
	}

	playerManage::color = { Colors[0], Colors[1], Colors[2], 255};

	Animate::updateAllAnimations();
}

void settingsUpdate(float dt) {
	ClearBackground(DARKGRAY);

	Vector2 mousePosition = GetMousePosition();

	// background grid
	if (lst != winWidth + winHeight) TextureCreated = false;
	lst = winWidth + winHeight;

	if (!TextureCreated) {
		TextureCreated = true;

		grid = LoadRenderTexture(winWidth * 1.2, winHeight * 1.2);

		BeginTextureMode(grid);

		for (int i = 0; i < 100; i++) {
			DrawLineEx({ (float)i * 75, -winHeight * 0.2f }, { (float)i * 75 , (float)winHeight * 1.2f }, 5, { 75,75,75,255 });
			DrawLineEx({ -winWidth * 0.2f, (float)i * 75 }, { (float)winWidth * 1.2f , (float)i * 75 }, 5, { 75,75,75,255 });
		}

		EndTextureMode();
	}

	DrawTexture(grid.texture, ((mousePosition.x / winWidth) - 0.5) * winWidth * 0.1 - winWidth * 0.1, ((mousePosition.y / winHeight) - 0.5) * winHeight * 0.1 - winHeight * 0.1, WHITE);
	DrawRectangle(0, 0, winWidth, winHeight, { 255,153,153,30 });

	//name
	float startHeight = winHeight * 0.00f;
	float startWidth = winWidth * 0.45f;
	DrawRectangle(startWidth, startHeight, winWidth * 0.1f, winHeight * 0.07f, { 255, 102, 102, 255 });

	DrawTriangle(
		{ startWidth, startHeight },
		{ startWidth - winWidth * 0.05f, startHeight },
		{ startWidth,  startHeight + winHeight * 0.07f },
		{ 255, 102, 102, 255 }
	);

	DrawTriangle(
		{ startWidth + winWidth * 0.1f, startHeight },
		{ startWidth + winWidth * 0.1f, startHeight + winHeight * 0.07f },
		{ startWidth + winWidth * 0.1f + winWidth * 0.05f, startHeight },
		{ 255, 102, 102, 255 }
	);

	DrawText("Settings", winWidth * 0.46, startHeight, getSizeOfText("Settings", { winWidth * 0.45f, startHeight, winWidth * 0.1f, winHeight * 0.07f }, 0), { 255, 204, 204, 255 });

	// back to menu
	DrawRectangle(0, winHeight * 0.93, winWidth * 0.1, winHeight * 0.07, { 255, 102, 102, 255 });
	DrawText("Menu", winWidth * 0.01, winHeight * 0.94, getSizeOfText("Menu", { 0, winHeight * 0.93f, winWidth * 0.1f, winHeight * 0.07f }, 0), { 255, 204, 204, 255 });

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		if (mousePosition.x > 0 and mousePosition.x < 0 + winWidth * 0.1 and mousePosition.y>winHeight * 0.93 and mousePosition.y < winHeight * 0.93 + winHeight * 0.07) {
			stateOfGame = gameState::MAIN;
		}
	}

	// binds
	auto b = [=]() {
		static int changingBind = -1;
		static KeyboardKey* binds[] = { &Settings::binds::ability, &Settings::binds::shot, &Settings::binds::pause };

		static ostringstream allNames; static bool inited = false; if (!inited) for (int i = 0; i < Settings::binds::bindsQuantity; i++) allNames << ((i % 13 == 0 and i != 0) ? "\n" : "") << Settings::binds::bindKeysName[i] << " "; inited = true;
		
		int startX = winWidth * 0.1;
		int startY = winHeight * 0.23;

		int SizeX = winWidth * 0.17;
		int SizeY = winHeight * 0.05;

		DrawText("Keybindings", startX, startY - winHeight * 0.07, getSizeOfText("Keybindings", { (float)startX, (float)startY, (float)SizeX, (float)SizeY }, 0), { 255,229,204,255 });

		for (int i = 0; i < 3; i++) {
			DrawRectangleRounded({ (float)startX, startY + SizeY * 1.3f * i, (float)SizeX, (float)SizeY }, 0.3, 5, { 255,229,204,255 });
			DrawRectangleRoundedLinesEx({ (float)startX, startY + SizeY * 1.3f * i, (float)SizeX, (float)SizeY }, 0.3, 5, winWidth * 0.001f, { 0,0,0,255 });
			DrawText(Settings::binds::bindsName[i].c_str(), startX + winWidth * 0.003, startY + SizeY * 1.3 * i + winHeight * 0.003, getSizeOfText(Settings::binds::bindsName[i].c_str(), { (float)startX, startY + SizeY * 1.3f * i, SizeX * 0.7f, (float)SizeY }, 0), { 0,0,0,255 });

			int index = 0;

			for (KeyboardKey key : Settings::binds::bindKeys) {
				if (key == *binds[i] or index > Settings::binds::bindsQuantity - 1) break;
				index++;
			}

			string txt = Settings::binds::bindKeysName[index];

			DrawText(txt.c_str(), startX + SizeX * 0.7f, startY + SizeY * 1.3 * i + winHeight * 0.003, getSizeOfText(txt.c_str(), { (float)startX * 0.7f, startY + SizeY * 1.3f * i, SizeX * 0.3f, (float)SizeY }, 0), { 0,0,0,255 });
		}

		if (changingBind != -1) {
			ostringstream c; c << "Press key to bind " << "\"" << Settings::binds::bindsName[changingBind] << "\"";
			DrawRectangleRounded({ winWidth * 0.3f, winHeight * 0.3f, winWidth * 0.4f, winHeight * 0.4f }, 0.1, 10, { 160,160,160,255 });
			DrawRectangleRoundedLinesEx({ winWidth * 0.3f, winHeight * 0.3f, winWidth * 0.4f, winHeight * 0.4f }, 0.1, 10, winWidth * 0.002f, { 0,0,0,255 });
			DrawText(c.str().c_str(), winWidth * 0.3 + winWidth * 0.005, winHeight * 0.31, getSizeOfText(c.str().c_str(), { winWidth * 0.3f, winHeight * 0.31f, winWidth * 0.4f, winHeight * 0.1f }, 0), { 0,0,0,255 });
			DrawText(allNames.str().c_str(), winWidth * 0.3 + winWidth * 0.005, winHeight * 0.5, getSizeOfText(allNames.str().c_str(), { winWidth * 0.3f, winHeight * 0.5f, winWidth * 0.4f, winHeight * 0.1f }, 0), { 0,0,0,255 });

			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) and (mousePosition.x < winWidth * 0.3 or mousePosition.x > winWidth * 0.7 or mousePosition.y < winHeight * 0.3 or mousePosition.y > winHeight * 0.7)) {
				changingBind = -1;
			}

			for (int i = 0; i < Settings::binds::bindsQuantity; i++) {
				if (IsKeyPressed(Settings::binds::bindKeys[i])) {
					bool z = true;

					for (int j = 0; j < 3; j++) {
						if (*binds[j] == Settings::binds::bindKeys[i] and j != changingBind) {
							z = false;
							break;
						}
					}

					if (z) {
						*(binds[changingBind]) = Settings::binds::bindKeys[i];
						changingBind = -1;
						break;
					}
				}
			}
		}
		else {
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				for (int i = 0; i < 3; i++) {
					if (mousePosition.x >= startX and mousePosition.x <= startX + SizeX and mousePosition.y >= startY + SizeY * 1.3 * i and mousePosition.y <= startY + SizeY * 1.3 * i + SizeY) {
						changingBind = i;
						break;
					}
				}
			}
		}
		};

	// game
	auto g = [=]() {
		static Vector3I* stgs[] = { &Settings::game::EatBallsQuantity, &Settings::game::BotsQuantity, &Settings::game::MapSize };

		int startX = winWidth * 0.4;
		int startY = winHeight * 0.23;

		int SizeX = winWidth * 0.23;
		int SizeY = winHeight * 0.05;

		DrawText("Game", startX, startY - winHeight * 0.07, getSizeOfText("Game", { (float)startX, (float)startY, (float)SizeX, (float)SizeY }, 0), { 204,255,255,255 });

		for (int i = 0; i < 3; i++) {

			float lrp = ((float)(*stgs[i]).x - (float)(*stgs[i]).y) / ((float)(*stgs[i]).z - (float)(*stgs[i]).y);

			DrawRectangleRounded({ (float)startX, startY + SizeY * 1.3f * i, (float)SizeX, (float)SizeY }, 0.01, 5, { 255,255,255,255 });
			DrawRectangleRounded({ (float)startX, startY + SizeY * 1.3f * i, (float)SizeX * lrp, (float)SizeY }, 0.01, 5, { 204,255,255,255 });
			DrawRectangleRoundedLinesEx({ (float)startX, startY + SizeY * 1.3f * i, (float)SizeX, (float)SizeY }, 0.01, 5, winWidth * 0.001f, { 0,0,0,255 });
			DrawText(Settings::game::settingsNames[i].c_str(), startX + winWidth * 0.003, startY + SizeY * 1.3 * i + winHeight * 0.003, getSizeOfText(Settings::game::settingsNames[i].c_str(), { (float)startX, startY + SizeY * 1.3f * i, SizeX * 0.7f, (float)SizeY }, 0), { 0,0,0,255 });

			ostringstream z; z << (*stgs[i]).x; 
			DrawText(z.str().c_str(), startX + SizeX * 0.75f, startY + SizeY * 1.3 * i + winHeight * 0.003, getSizeOfText(z.str().c_str(), { (float)startX + SizeX * 0.05f, startY + SizeY * 1.3f * i, (float)SizeX * 0.25f, (float)SizeY }, 0), {0,0,0,255});
		}

		static int downed = -1;

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			for (int i = 0; i < 3; i++) {
				if (mousePosition.x >= startX and mousePosition.x <= startX + SizeX and mousePosition.y >= startY + SizeY * 1.3f * i and mousePosition.y <= startY + SizeY * 1.3f * i + SizeY) {
					downed = i;
					break;
				}
			}
		}

		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
			downed = -1;
		}

		if (downed != -1) {
			float v = (mousePosition.x - startX) / SizeX; if (v > 1) v = 1; if (v < 0) v = 0;
			(*stgs[downed]).x = lerp((*stgs[downed]).y, (*stgs[downed]).z, v);
		}
		
		};

	// graphic
	auto v = [=]() {
		static bool* stgs[] = { &Settings::graphic::abilityEffects, &Settings::graphic::drawGrid, &Settings::graphic::renderEat, &Settings::graphic::playersBorder };

		int startX = winWidth * 0.7;
		int startY = winHeight * 0.23;

		int SizeX = winWidth * 0.25;
		int SizeY = winHeight * 0.05;

		float lrp = 1; for (int i = 0; i < 4; i++) lrp -= *stgs[i] * 0.25;

		DrawText("Video", startX, startY - winHeight * 0.07, getSizeOfText("Video", { (float)startX, (float)startY, (float)SizeX, (float)SizeY }, 0), lerp({ 229,255,204,255 }, { 255,204,204,255 }, lrp));

		for (int i = 0; i < 4; i++) {
			Color ZZ = { 255, 204, 204, 255 };
			((*stgs[i]) ? ZZ = { 229, 255, 204, 255 } : ZZ = { 255, 204, 204, 255 });
			DrawRectangleRounded({ (float)startX, startY + SizeY * 1.3f * i, (float)SizeX, (float)SizeY }, 0.01, 5, ZZ);
			DrawRectangleRoundedLinesEx({ (float)startX, startY + SizeY * 1.3f * i, (float)SizeX, (float)SizeY }, 0.01, 5, winWidth * 0.001f, { 0,0,0,255 });
			DrawText(Settings::graphic::settingsNames[i].c_str(), startX + winWidth * 0.003, startY + SizeY * 1.3 * i + winHeight * 0.003, getSizeOfText(Settings::game::settingsNames[i].c_str(), { (float)startX, startY + SizeY * 1.3f * i, (float)SizeX * 0.75f, (float)SizeY }, 0), { 0,0,0,255 });

			string z = (*stgs[i] ? "True" : "False");
			DrawText(z.c_str(), startX + SizeX * 0.75f, startY + SizeY * 1.3 * i + winHeight * 0.003, getSizeOfText(z.c_str(), { (float)startX + SizeX * 0.75f, startY + SizeY * 1.3f * i, (float)SizeX * 0.25f, (float)SizeY }, 0), { 0,0,0,255 });
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			for (int i = 0; i < 4; i++) {
				if (mousePosition.x >= startX and mousePosition.x <= startX + SizeX and mousePosition.y >= startY + SizeY * 1.3f * i and mousePosition.y <= startY + SizeY * 1.3f * i + SizeY) {
					*stgs[i] = !*stgs[i];
					break;
				}
			}
		}

		};

	g(); v(); b();
}

int main() {
	Vector3 screenData = getScreenData();
	screenRate = 0; //static_cast<int>(screenData.z);
	winWidth = static_cast<int>(screenData.x) / 1.2;
	winHeight = static_cast<int>(screenData.y) / 1.2;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(winWidth, winHeight, "Agar.io");

	SetTargetFPS(screenRate);
	SetExitKey(KEY_NULL);

	//startGame();

	while (!WindowShouldClose()) {
		winWidth = GetScreenWidth();
		winHeight = GetScreenHeight();
		float dt = GetFrameTime();
		BeginDrawing();

		switch (stateOfGame) {
		case (gameState::GAMEPLAY): gameplayUpdate(dt); break;
		case (gameState::MAIN): mainUpdate(dt); break;
		case (gameState::SHOP): characterUpdate(dt); break;
		case (gameState::SETTINGS): settingsUpdate(dt); break;
		case (gameState::GAMEPLAY_PAUSED): gameplayUpdate(dt); break;
		}

		EndDrawing();
	}

	for (Texture2D t : abilityTextures) {
		UnloadTexture(t);
	}

	CloseWindow();

	return 0;
}