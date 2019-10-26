#include "player.hpp"

using namespace std;

Player::Player(string name, PlayerState *playerState, int position) {
	this->setName(name);
	this->playerState = playerState;
	this->setPosition(position);
};

Player::~Player() {};

string Player::getName() {
	return this->name;
};

void Player::setName(string name) {
	this->name = name;
};

int Player::getPosition() {
	return this->position;
};

void Player::setPosition(int position) {
	this->position = position;
};

bool Player::isInJail() {
	return this->playerState->getIsInJail();
}

bool Player::isSolvent(int rent) {
	// narazie jest sprawdzany tylko warunek posiadanych pieniedzy
	// do tego trzeba dodac czy wartosc majątku > rent
	if (this->playerState->getMoney() > rent) {
		return true;
	}
	else
	{
		return false;
	}
	
}