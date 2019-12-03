#include "gamecontroller.hpp"
#include "input.hpp"
#include <string>
#include <iostream>
#include <utility>
#include <algorithm>

using namespace std;

// struktura odpowiada za procedure porownania par <Player, int> w celu posortowania wektora z kolejnoscia graczy
struct moveOrder {
    inline bool operator() (const pair<Player, int>& firstPair, const pair<Player, int>& secondPair) {
        return (firstPair.second > secondPair.second);
    }
};

GameController::GameController(Board* board, DiceRoller* diceRoller, int numberOfPlayers, Menu* menu) {
    this->board = board;
    this->diceRoller = diceRoller;
    this->numberOfPlayers = numberOfPlayers;
    this->numberOfActivePlayers = numberOfPlayers;
    this->menu = menu;

    string names[] = {"Kamil", "Wojciech", "Adrian", "Pawel"};

    for(int i = 0; i < this->numberOfPlayers; i++) {
        bool isComputer = i == 0 ? false : true; // przy 1 obiegu petli i == 0 zatem 1 utworzony gracz to gracz, reszta to komputery

        Player* player = new Player(names[i], isComputer, new PlayerState(1500, false, false, 0), 0);

        this->players.push_back(*player);
    }

    // this->currentPlayer = &this->players[0];
}

GameController::~GameController() {
    this->renderMessage("Usuwam obiekt GameController");
    this->players.clear();
    this->orderOfMoves.clear();

    delete this->board;
    delete this->diceRoller;  
    delete this->currentPlayer;  
    delete this->menu;
}

void GameController::start() {
    this->setPlayersMoveOrder();    
	this->renderPlayersMoveOrder();
    this->setPlayersOnStart();

    bool isPlaying = true;
	pickBlueCard(currentPlayer);
    while(isPlaying && this->numberOfActivePlayers > 1) {
        
        this->renderCurrentPlayer();

        if (this->currentPlayer->isComputer()) {
            cout << "KOMPUTER WYKONUJE RUCH" << endl;
        }

        this->menu->render(this->currentPlayer); // wyświetlenie menu
        int playerChose = Input::getDigitKey();

        if (this->currentPlayer->isInJail()) {
            if (playerChose == 9) {
                isPlaying = false;
                break;
            }
            else if (playerChose == 0) {
                // GRACZ WYBRAL RZUT KOSTKAMI
                this->renderMessage("RZUCAM KOSTKAMI W WIEZIENIU");
                this->getOutFromJailDiceRoll();
                this->renderPlayersPositions();
                this->nextPlayer();
            }
            else if (playerChose == 1) {
                // GRACZ WYBRAL UZYCIE KARTY
                this->renderMessage("UZYWAM KARTY");
            }
            else if (playerChose == 2) {
                // GRACZ WYBRAL ZAPLATE I RZUT KOSTKAMI
                this->renderMessage("PLACE I RZUCAM");
            }
            else {
                this->renderMessage("Niepoprawny symbol");
                continue;
            }
        }
        else {       
            if (playerChose == 9) {
                isPlaying = false;
                break;
            }
            else if (playerChose == 0) {
                // GRACZ WYBRAL RZUT KOSTKAMI
                this->renderMessage("RZUCAM KOSTKAMI");
                this->normalDiceRoll();
                this->renderPlayersPositions();
                this->nextPlayer();
            }
            else if (playerChose == 1) {
                // GRACZ WYBRAL ZAKUP DOMKOW
                this->renderMessage("KUPUJE DOMKI");
            }
            else if (playerChose == 2) {
                // GRACZ WYBRAL WYMIANE
                this->renderMessage("WYMIENIAM");
            }
            else if (playerChose == 3) {
                // GRACZ WYBRAL WZIECIE POZYCZKI Z BANKU
                this->takeLoan(this->currentPlayer);                
            }
            else {
                this->renderMessage("Niepoprawny symbol");
                continue;
            }
        } 
    }
}

void GameController::setPlayersOnStart() {
    for (int index = 0; index < this->players.size(); index++) {
        this->players[index].setPosition(0);
    }
}

// metoda przesuwa wskaznik na nastepnego gracza ktory bedzie wykonywal ruch
// jezeli wskaznik wskazuje na ostatni element vectora orderOfMoves to przesun go na pierwszy element
void GameController::nextPlayer() {

    if (this->currentPlayer == &this->orderOfMoves[this->numberOfActivePlayers - 1]) {
        this->currentPlayer = &this->orderOfMoves[0];
    }
    else {
        this->currentPlayer++;
    }
}

bool GameController::doesSomeoneWin() {
    // tymczasowe implementacja
    return false;
}

void GameController::performAction() {
    int indexOfFieldContext = this->currentPlayer->getPosition();

    Field* contextField = this->board->getField(indexOfFieldContext);

    switch(contextField->getFieldType()) {
        case START:
            // this->currentPlayer->earMoneyFromStart();
            // Obsluga tego pola jest juz zaimplementowana w Player MoveBy
            break;
        case JAIL:
            this->renderMessage("Odwiedzasz wiezienie");
            break;
        case GO_TO_JAIL:
            this->renderMessage("Idziesz do wiezienia!");
            this->currentPlayer->goToJail();
            break;
        case FREE_PARKING:
            this->renderMessage("Odpoczywasz na darmowym parkingu");
            break;
        case LUXURY_TAX:
            if(this->currentPlayer->isSolvent(100, true)) {
                this->currentPlayer->payMoney(100);
            }
            else {
                this->bankruptPlayer(this->currentPlayer);
            }
            break;
        case INCOME_TAX:
            if(this->currentPlayer->isSolvent(200, true)) {
                this->currentPlayer->payMoney(200);
            }
            else
            {
                this->bankruptPlayer(this->currentPlayer);
            }
            break;
            
    }
}

void GameController::takeLoan(Player* player) {
    this->renderMessage("BIORE POZYCZKE");
    
    if (player->hasActiveLoan()) {
        this->renderMessage("NIE MOZESZ WZIAC POZYCZKI DO POKI NIE SPLACISZ POPRZEDNIEJ");

        if (!player->isSolvent(500, true)) {
            this->renderMessage("NIE STAC CIE NA SPLATE POZYCZKI");
            return;            
        }

        this->renderMessage("MOZESZ SPLACIC POZYCZKE \n 1 - SPLAC \n 2 - ANULUJ");
        int playerChose = Input::getDigitKey();

        if(playerChose == 1) {
            player->payBackLoan();
            this->renderMessage("POZYCZKA SPŁACONA");
        }
    }
    else {
        player->takeLoan();
        this->renderMessage("WZIETO POZYCZKE");
    }
}

// Prosty rzut kostkami - rzut -> przesunięcie gracza
void GameController::simpleDiceRoll() {
    DiceThrowResult* dtr = this->diceRoller->rollDices();
    int rolledNumber = dtr->firstDice + dtr->secondDice;
    this->currentPlayer->moveBy(rolledNumber);
    this->renderPlayersPositions();
}

void GameController::normalDiceRoll() {
    int doubles = 0;
    int rolledNumber = 0;
    DiceThrowResult* dtr = NULL;

    while(true) {
        dtr = this->diceRoller->rollDices();
        rolledNumber = dtr->firstDice + dtr->secondDice;

        if (dtr->isDouble()) {
            this->renderMessage("WYRZUCONO DEBEL");
            doubles++;

            if (doubles == 3) {
                this->renderMessage("IDZIESZ DO WIEZIENIA");
                this->currentPlayer->goToJail();
                return;
            }

            this->currentPlayer->moveBy(rolledNumber);

            // WYKONAJ AKCJE DLA POLA
            this->performAction();
        }
        else {
            this->currentPlayer->moveBy(rolledNumber);
            // WYKONAJ AKCJE DLA POLA
            this->performAction();
            return; 
        }
    }
}

void GameController::getOutFromJailDiceRoll() {
    DiceThrowResult* dtr = NULL;

    for(int roll = 0; roll < 3; roll++) {
        dtr = this->diceRoller->rollDices();

        if (dtr->isDouble()) {
            int rolledNumber = dtr->firstDice + dtr->secondDice;
            this->currentPlayer->getOutOfJail();
            this->currentPlayer->moveBy(rolledNumber);

            return;
        }
    }
}

void GameController::bankruptPlayer(Player* player) {
    player->setBankrupt(true);
    this->numberOfActivePlayers--;
                    
    for(int index = 0; index < this->orderOfMoves.size(); index++) {
        if (player == &this->orderOfMoves[index]) {
            this->nextPlayer();
            this->orderOfMoves.erase(this->orderOfMoves.begin() + index);
            break;
        }
    }
}

// kazdy z graczy rzuca kostkami, na podstawie wynikow ustalana jest kolejnosc wykonywania ruchow
// na koncu nastepuje ustawienie wskaznika na pierwszego gracza ktory bedzie wykonywal ruch
// PRZEZ TO ZE SA PRZEKAZYWANE WARTOSCI TO vector orderOfMoves przechowuje inne obiekty niz vetor players
// DO POPRAWY
void GameController::setPlayersMoveOrder() {
    vector<pair<Player, int>> playerRolls;

    this->currentPlayer = &this->players[0];

    for(int i = 0; i < this->players.size(); i++) {
        DiceThrowResult* dtr = this->diceRoller->rollDices();
        int rolledNumber = dtr->firstDice + dtr->secondDice;
        pair<Player, int> currentRoll (*this->currentPlayer, rolledNumber);
        playerRolls.push_back(currentRoll);
        this->currentPlayer++;
    }

    sort(playerRolls.begin(), playerRolls.end(), moveOrder());

    for(vector<pair<Player, int>>::iterator it = playerRolls.begin();
        it != playerRolls.end(); 
        ++it) {
            this->orderOfMoves.push_back(it->first);
    }

    this->currentPlayer = &this->orderOfMoves[0];
}

void GameController::renderPlayersMoveOrder() {
    this->renderMessage("Gracze wykonuja ruch w kolejnosci: ");
   
    for(vector<Player>::iterator it = this->orderOfMoves.begin(); it != this->orderOfMoves.end(); ++it) {
        this->renderMessage(it->getName());
    }

    cout << endl;
}

// wypisuje na konsole aktualna pozycje kazdego z graczy
void GameController::renderPlayersPositions() {
    for(vector<Player>::iterator it = this->orderOfMoves.begin(); it != this->orderOfMoves.end(); ++it) {
        cout << "Gracz " << it->getName() 
             << " jest na polu " << it->getPosition() 
             << endl;
    }
    
    cout << endl;
}

void GameController::renderCurrentPlayer() {
    this->renderMessage("Ruch gracza: " + this->currentPlayer->getName());
    cout << "Stan konta: " << this->currentPlayer->getPlayerState().getMoney() << endl;
    cout << endl;
}

void GameController::renderMessage(string message) {
    cout << message << endl;
}

void GameController::pickBlueCard(Player* player) {
	//wylosowanie karty niebieskiej
	Card card = board->pickBlueCard();
	switch (card.getCardId())
	{
	case 0:
		//Wychodzisz wolny z więzienia. Kartę należy zachować do wykorzystania lub sprzedania.
		player->addOutOfJailCard();
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 1:
		//Wracasz do \"WIEDNIA\"
		player->setPosition(39);
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 2:
		//Płacisz za karę 20$ lub ciągniesz \"SZANSĘ\" z drugiego zestawu (czerwonego)
		renderMessage("BLUE CARD: " + card.getDescription());
		//jeśli niezdolny zapłacić automatycznie ciągnie karte
		if (player->getPlayerState().getMoney() < 20) {
			pickRedCard(player);
			//break or return?
		}
		else {
			renderMessage("0 By zapłacić karę");
			renderMessage("1 By pobrać partę z zestawu czerwonego");
			int playerChose = Input::getDigitKey();
			switch (playerChose)
			{
			case 0:
				player->getPlayerState().setMoney(player->getPlayerState().getMoney()-20);
			case 1:
				pickRedCard(player);
			default:
				break;
			}
		}
		break;
	case 3:
		//Wracasz na \"START\"
		player->setPosition(0);
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 4:
		//Idziesz do więzienia.Nie przechodzisz przez \"START\". Nie otrzymujesz 200$.
		player->setPosition(10);
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 5:
		//Płacisz koszty leczenia w wysokości 20$.
		
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 6:
		//Bank omylił się na Twoją krozyść. Otrzymujesz 400$.
		player->getPlayerState().setMoney(player->getPlayerState().getMoney()+400);
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 7:
		//Zająłeś II miejsce w konkursie piękności - otrzymujesz z banku 200$.
		player->getPlayerState().setMoney(player->getPlayerState().getMoney() + 200);
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 8:
		//Otrzymujesz roczną rentę w wysokości 200$.
		player->getPlayerState().setMoney(player->getPlayerState().getMoney() + 200);
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 9:
		//Bank wypłaci ci należne 7% od kapitałów - otrzymujesz 50$.
		player->getPlayerState().setMoney(player->getPlayerState().getMoney() + 50);
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 10:
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 11:
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 12:
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 13:
		//Otrzymujesz w spadku 200$.
		player->getPlayerState().setMoney(player->getPlayerState().getMoney() + 200);
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	case 14:
		renderMessage("BLUE CARD: " + card.getDescription());
		break;
	default:
		break;
	}
}

void GameController::pickRedCard(Player* player) {
	//wylosowanie kartty czerwonej
	Card card = board->pickRedCard();
	switch (card.getCardId())
	{
	case 0:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 1:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 2:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 3:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 4:
		//Cofasz się o 3 pola.
		player->setPosition(player->getPosition() - 3);
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 5:
		//Wcyhodzisz wolny z więzienia. Kartę należy zachować do wykorzystania lub sprzedania.
		player->addOutOfJailCard();
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 6:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 7:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 8:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 9:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 10:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 11:
		//Bank wpłaca Ci należne odsetkiw  wysokości 300$.
		player->getPlayerState().setMoney(player->getPlayerState().getMoney() + 300);
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 12:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 13:
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 14:
		//Bank wypłaca Ci pprocent w wysokości 100$.
		player->getPlayerState().setMoney(player->getPlayerState().getMoney() + 100);
		renderMessage("RED CARD: " + card.getDescription());
		break;
	case 15:
		//Rozwiązałeś dobrze krzyżówkę. Jako I nagrodę otrzymujesz 200$.
		player->getPlayerState().setMoney(player->getPlayerState().getMoney() + 200);
		renderMessage("RED CARD: " + card.getDescription());
		break;
	default:
		break;
	}
}
